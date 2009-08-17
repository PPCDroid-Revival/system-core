/* implement gdbserver service */
/* this file is derived from jdwp service implementation*/
#include "sysdeps.h"
#define  TRACE_TAG   TRACE_JDWP
#include "adb.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/* here's how these things work.

   when adbd starts, it creates a unix server socket
   named @gdb-debug-control (@ is a shortcut for "first byte is zero"
   to use the private namespace instead of the file system)
*/

#if !ADB_HOST

#include <sys/socket.h>
#include <sys/un.h>

typedef struct GdbProcess  GdbProcess;
struct GdbProcess {
    GdbProcess*  next;
    GdbProcess*  prev;
    int           pid;
    int           socket;
    fdevent*      fde;

    char          in_buff[4];  /* input character to read PID */
    int           in_len;      /* number from GDBed process    */

};

static GdbProcess  _gdb_list;

static int
gdb_process_list( char*  buffer, int  bufferlen )
{
    char*         end  = buffer + bufferlen;
    char*         p    = buffer;
    GdbProcess*  proc = _gdb_list.next;

    for ( ; proc != &_gdb_list; proc = proc->next ) {
        int  len;

        /* skip transient connections */
        if (proc->pid < 0)
            continue;

        len = snprintf(p, end-p, "%d\n", proc->pid);
        if (p + len >= end)
            break;
        p += len;
    }
    p[0] = 0;
    return (p - buffer);
}


static void
gdb_process_free( GdbProcess*  proc )
{
    if (proc) {
        int  n;

        proc->prev->next = proc->next;
        proc->next->prev = proc->prev;

        if (proc->socket >= 0) {
            shutdown(proc->socket, SHUT_RDWR);
            adb_close(proc->socket);
            proc->socket = -1;
        }

        if (proc->fde != NULL) {
            fdevent_destroy(proc->fde);
            proc->fde = NULL;
        }
        proc->pid = -1;

        free(proc);
    }
}


static void  gdb_process_event(int, unsigned, void*);  /* forward */


static GdbProcess*
gdb_process_alloc( int  socket )
{
    GdbProcess*  proc = calloc(1,sizeof(*proc));

    if (proc == NULL) {
        D("not enough memory to create new GDB process\n");
        return NULL;
    }

    proc->socket = socket;
    proc->pid    = -1;
    proc->next   = proc;
    proc->prev   = proc;

    proc->fde = fdevent_create( socket, gdb_process_event, proc );
    if (proc->fde == NULL) {
        D("could not create fdevent for new GDB process\n" );
        free(proc);
        return NULL;
    }

    proc->fde->state |= FDE_DONT_CLOSE;
    proc->in_len      = 0;

    /* append to list */
    proc->next = &_gdb_list;
    proc->prev = proc->next->prev;

    proc->prev->next = proc;
    proc->next->prev = proc;

    /* start by waiting for the PID */
    fdevent_add(proc->fde, FDE_READ);

    return proc;
}


static void gdb_run_gdbserver(GdbProcess *proc);

static void
gdb_process_event( int  socket, unsigned  events, void*  _proc )
{
    GdbProcess*  proc = _proc;

    if (events & FDE_READ) {
        if (proc->pid < 0) {
            /* read the PID as a 4-hexchar string */
            char*  p    = proc->in_buff + proc->in_len;
            int    size = 4 - proc->in_len;
            char   temp[5];
            while (size > 0) {
                int  len = recv( socket, p, size, 0 );
                if (len < 0) {
                    if (errno == EINTR)
                        continue;
                    if (errno == EAGAIN)
                        return;
                    /* this can fail here if the GDB process crashes very fast */
                    D("weird unknown GDB process failure: %s\n",
                      strerror(errno));

                    goto CloseProcess;
                }
                if (len == 0) {  /* end of stream ? */
                    D("weird end-of-stream from unknown GDB process\n");
                    goto CloseProcess;
                }
                p            += len;
                proc->in_len += len;
                size         -= len;
            }
            /* we have read 4 characters, now decode the pid */
            memcpy(temp, proc->in_buff, 4);
            temp[4] = 0;

            if (sscanf( temp, "%04x", &proc->pid ) != 1) {
                D("could not decode GDB %p PID number: '%s'\n", proc, temp);
                goto CloseProcess;
            }
	    gdb_run_gdbserver(proc);

        }
        else
        {
            /* the pid was read, if we get there it's probably because the connection
             * was closed (e.g. the GDBed process exited or crashed) */
            char  buf[32];

            for (;;) {
                int  len = recv(socket, buf, sizeof(buf), 0);

                if (len <= 0) {
                    if (len < 0 && errno == EINTR)
                        continue;
                    if (len < 0 && errno == EAGAIN)
                        return;
                    else {
                        D("terminating GDB %d connection: %s\n", proc->pid,
                          strerror(errno));
                        break;
                    }
                }
                else {
                    D( "ignoring unexpected GDB %d control socket activity (%d bytes)\n",
                       proc->pid, len );
                }
            }

        CloseProcess:
            if (proc->pid >= 0)
                D( "remove pid %d to gdb process list\n", proc->pid );
            gdb_process_free(proc);
            return;
        }
    }
}


/**  GDB CONTROL SOCKET
 **
 **  we do implement a custom asocket to receive the data
 **/

/* name of the debug control Unix socket */
#define  GDB_CONTROL_NAME      "\0gdb-control"
#define  GDB_CONTROL_NAME_LEN  (sizeof(GDB_CONTROL_NAME)-1)

typedef struct {
    int       listen_socket;
    fdevent*  fde;

} GdbControl;


static void
gdb_control_event(int  s, unsigned events, void*  user);


static int
gdb_control_init( GdbControl*  control,
                   const char*   sockname,
                   int           socknamelen )
{
    struct sockaddr_un   addr;
    socklen_t            addrlen;
    int                  s;
    int                  maxpath = sizeof(addr.sun_path);
    int                  pathlen = socknamelen;

    if (pathlen >= maxpath) {
        D( "vm debug control socket name too long (%d extra chars)\n",
           pathlen+1-maxpath );
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, sockname, socknamelen);

    s = socket( AF_UNIX, SOCK_STREAM, 0 );
    if (s < 0) {
        D( "could not create vm debug control socket. %d: %s\n",
           errno, strerror(errno));
        return -1;
    }

    addrlen = (pathlen + sizeof(addr.sun_family));

    if (bind(s, (struct sockaddr*)&addr, addrlen) < 0) {
        D( "could not bind vm debug control socket: %d: %s\n",
           errno, strerror(errno) );
        adb_close(s);
        return -1;
    }

    if ( listen(s, 4) < 0 ) {
        D("listen failed in gdb control socket: %d: %s\n",
          errno, strerror(errno));
        adb_close(s);
        return -1;
    }

    control->listen_socket = s;

    control->fde = fdevent_create(s, gdb_control_event, control);
    if (control->fde == NULL) {
        D( "could not create fdevent for gdb control socket\n" );
        adb_close(s);
        return -1;
    }

    /* only wait for incoming connections */
    fdevent_add(control->fde, FDE_READ);

    D("gdb control socket started (%d)\n", control->listen_socket);
    return 0;
}


static void
gdb_control_event( int  s, unsigned  events, void*  _control )
{
    GdbControl*  control = (GdbControl*) _control;

    if (events & FDE_READ) {
        struct sockaddr   addr;
        socklen_t         addrlen = sizeof(addr);
        int               s = -1;
        GdbProcess*      proc;

        do {
            s = adb_socket_accept( control->listen_socket, &addr, &addrlen );
            if (s < 0) {
                if (errno == EINTR)
                    continue;
                if (errno == ECONNABORTED) {
                    /* oops, the GDB process died really quick */
                    D("oops, the GDB process died really quick\n");
                    return;
                }
                /* the socket is probably closed ? */
                D( "weird accept() failed on gdb control socket: %s\n",
                   strerror(errno) );
                return;
            }
        }
        while (s < 0);

        proc = gdb_process_alloc( s );
        if (proc == NULL)
            return;
    }
}

static void gdb_run_gdbserver(GdbProcess *proc) 
{
	char const *gdbserver_args[5];
	char pidbuf[16];
	int rc;

        rc = fork();
        if ( rc < 0 ) {
                printf ("Fork failed!\n");
		return;
        }
        if ( rc == 0 ) {
                sprintf(pidbuf,"%d",proc->pid);
                gdbserver_args[0]="/system/bin/gdbserver";
                gdbserver_args[1]=":10000";
                gdbserver_args[2]="--attach";
                gdbserver_args[3]=pidbuf;
                gdbserver_args[4]=0;
                rc = execvp(gdbserver_args[0],(char * const *)gdbserver_args);
                printf ("execvp()=%d\n",rc);
                exit(0);
        }

}

static GdbControl   _gdb_control;

int
init_gdb(void)
{
    _gdb_list.next = &_gdb_list;
    _gdb_list.prev = &_gdb_list;

    return gdb_control_init( &_gdb_control,
                              GDB_CONTROL_NAME,
                              GDB_CONTROL_NAME_LEN );
}

#endif /* !ADB_HOST */

