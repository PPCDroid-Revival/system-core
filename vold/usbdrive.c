/*
 * Copyright (C) 2009 The Android-x86 Open Source Project
 *
 * Author: Luke Chen <jschen.cse@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <sys/types.h>

#include "vold.h"
#include "usbdrive.h"
#include "media.h"


#define DEBUG_USB_BOOTSTRAP 1

static int usb_bootstrap_controller(const char *sysfs_path);
static int usb_bootstrap_target(const char *sysfs_path, boolean is_device);
static int usb_bootstrap_device(const char *sysfs_path);
static int usb_bootstrap_block(const char *sysfs_path);
static int usb_bootstrap_sdx(const char *sysfs_path);
static int usb_bootstrap_sdx_partition(const char *sysfs_path);

/*
 * Bootstrap our usb information.
 */
int usb_bootstrap()
{
    DIR *d;
    struct dirent *de;

    if (!(d = opendir(SYSFS_CLASS_USB_PATH))) {
        LOG_ERROR("Unable to open '%s' (%s)", SYSFS_CLASS_USB_PATH,
                  strerror(errno));
        return -errno;
    }

    while ((de = readdir(d)))
        if (de->d_name[0] != '.') {
            char tmp[SYSFS_PATH_MAX];
            sprintf(tmp, "%s/%s", SYSFS_CLASS_USB_PATH, de->d_name);
            if (usb_bootstrap_controller(tmp)) {
                LOG_ERROR("Error bootstrapping controller '%s' (%s)", tmp,
                          strerror(errno));
            }
        }

    closedir(d);

    return 0;
}

static int usb_bootstrap_controller(const char *sysfs_path)
{
    DIR *d;
    struct dirent *de;
    char buffer[SYSFS_PATH_MAX];

#if DEBUG_USB_BOOTSTRAP
    LOG_VOL("bootstrap_controller(%s):", sysfs_path);
#endif

    /*
     * FIXME:
     * Is it correct to know if the device is usb storage by checking /sys/class/scsi_host/hostX/proc_name ?
     */
    read_sysfs_var(buffer, sizeof(buffer), sysfs_path + 4, "proc_name");
    if (strncmp(buffer, "usb-storage", 11)) {
        LOG_VOL("%s is not a usb-storage", sysfs_path);
        return 0;
    }

    sprintf(buffer, "%s/%s", sysfs_path, "device");
    return usb_bootstrap_target(buffer, false);
}

static int usb_bootstrap_target(const char *sysfs_path, boolean is_device)
{
    DIR *d;
    struct dirent *de;
    char buffer[SYSFS_PATH_MAX];
    char *bufblock;

#if DEBUG_USB_BOOTSTRAP
    LOG_VOL("bootstrap_target(%s):", sysfs_path);
#endif

    if (!(d = opendir(sysfs_path))) {
        LOG_ERROR("Unable to open '%s' (%s)", sysfs_path, strerror(errno));
        return -errno;
    }

    strcpy(buffer, sysfs_path);
    bufblock = buffer + strlen(buffer);
    *bufblock++ = '/';

    while ((de = readdir(d)))
        if ((de->d_name[0] != '.') &&
                strcmp(de->d_name, "uevent") &&
                strcmp(de->d_name, "subsystem") &&
                strcmp(de->d_name, "scsi_host") &&
                strcmp(de->d_name, "power")) {
            strcpy(bufblock, de->d_name);
            if (is_device ? usb_bootstrap_device(buffer) : usb_bootstrap_target(buffer, true)) {
                LOG_ERROR("Error bootstrapping device '%s' (%s)", buffer, strerror(errno));
            }
        }

    closedir(d);
    return 0;
}

static int usb_bootstrap_device(const char *sysfs_path)
{
    char saved_cwd[SYSFS_PATH_MAX];
    char new_cwd[SYSFS_PATH_MAX];
    char tmp[SYSFS_PATH_MAX];
    char *uevent_params[2];

#if DEBUG_USB_BOOTSTRAP
    LOG_VOL("bootstrap_device(%s):", sysfs_path);
#endif

    /*
     * sysfs_path is based on /sys/class, but we want the actual device class
     */
    if (!getcwd(saved_cwd, sizeof(saved_cwd))) {
        LOGE("Error getting working dir path");
        return -errno;
    }

    if (chdir(sysfs_path) < 0) {
        LOGE("Unable to chdir to %s (%s)", sysfs_path, strerror(errno));
        return -errno;
    }

    if (!getcwd(new_cwd, sizeof(new_cwd))) {
        LOGE("Buffer too small for device path");
        return -errno;
    }

    if (chdir(saved_cwd) < 0) {
        LOGE("Unable to restore working dir");
        return -errno;
    }

    /*
     * Collect parameters so we can simulate a UEVENT
     */
    sprintf(tmp, "DEVPATH=%s", new_cwd + 4);
    uevent_params[0] = tmp;
    uevent_params[1] = NULL;
    if (simulate_uevent("scsi", new_cwd + 4, "add", uevent_params) < 0) {
        LOGE("Error simulating uevent (%s)", strerror(errno));
        return -errno;
    }

    /*
     *  Check for block drivers
     */
    if (!access(strcat(new_cwd, "/block"), F_OK)) {
        if (usb_bootstrap_block(new_cwd)) {
            LOGE("Error bootstrapping block @ %s", tmp);
        }
    }

    return 0;
}

static int usb_bootstrap_block(const char *sysfs_path)
{
    DIR *d;
    struct dirent *de;

#if DEBUG_USB_BOOTSTRAP
    LOG_VOL("usb_bootstrap_block(%s):", sysfs_path);
#endif

    if (!(d = opendir(sysfs_path))) {
        LOGE("Failed to opendir %s", sysfs_path);
        return -errno;
    }

    while ((de = readdir(d)))
        if (de->d_name[0] != '.') {
            char tmp[SYSFS_PATH_MAX];
            sprintf(tmp, "%s/%s", sysfs_path, de->d_name);
            if (usb_bootstrap_sdx(tmp)) {
                LOGE("Error bootstraping sdx @ %s", tmp);
            }
        }

    closedir(d);
    return 0;
}

static int usb_bootstrap_sdx(const char *sysfs_path)
{
    DIR *d;
    struct dirent *de;
    char *sdx_devname;
    ssize_t sz;
    char buffer[SYSFS_PATH_MAX];
    char *bufsdx;

#if DEBUG_USB_BOOTSTRAP
    LOG_VOL("usb_bootstrap_sdx(%s):", sysfs_path);
#endif

    if (usb_bootstrap_sdx_partition(sysfs_path)) {
        LOGE("Error bootstrapping sdx partition '%s'", sysfs_path);
        return -errno;
    }

    if (!(d = opendir(sysfs_path))) {
        LOGE("Failed to opendir %s", sysfs_path);
        return -errno;
    }

    sdx_devname = strrchr(sysfs_path, '/') + 1;
    sz = strlen(sdx_devname);
    strcpy(buffer, sysfs_path);
    bufsdx = buffer + strlen(buffer);
    *bufsdx++ = '/';

    while ((de = readdir(d)))
        if (de->d_type == DT_DIR && !strncmp(de->d_name, sdx_devname, sz)) {
            strcpy(bufsdx, de->d_name);
            if (!access(buffer, F_OK) && usb_bootstrap_sdx_partition(buffer)) {
                LOGE("Error bootstrapping sdx partition '%s'", buffer);
            }
        }

    closedir(d);
    return 0;
}

static int usb_bootstrap_sdx_partition(const char *sysfs_path)
{
    int i;
    char *uevent_params[5];
    char tmp[SYSFS_PATH_MAX];
    FILE *fp;
    char line[1024];
    char *uevent, *saveptr;
    const char *devpath = sysfs_path + 4;

#if DEBUG_USB_BOOTSTRAP
    LOG_VOL("usb_bootstrap_sdx_partition(%s):", devpath);
#endif

    if (!read_sysfs_var(line, sizeof(line), devpath, "uevent")) {
        LOGE("Unable to open '%s/%s' (%s)", sysfs_path, "uevent", strerror(errno));
        return -errno;
    }
    sprintf(tmp, "DEVPATH=%s", devpath);
    uevent_params[0] = tmp;
    for (i = 1, uevent = line; i < 4; ++i, uevent = NULL)
        if (!(uevent_params[i] = strtok_r(uevent, "\n", &saveptr)))
            break;
    uevent_params[i] = NULL;

    if (simulate_uevent("block", tmp + 8, "add", uevent_params) < 0) {
        LOGE("Error simulating uevent (%s)", strerror(errno));
        return -errno;
    }
    return 0;
}
