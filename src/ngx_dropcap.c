//SPDX-License-Identifier: AGPL-3.0-only
/*
 * Copyright (C) 2026 Marko Zajc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ngx_core.h>
#include <grp.h>
#include <linux/capability.h>
#include <syscall.h>
#include <unistd.h>

static ngx_int_t ngx_dropcap_has_supplementary() { // NOSONAR
	gid_t gid = getgid();
	gid_t groups[1];
	int rc = getgroups(1, groups);
	if (rc == -1) {
		if (errno == EINVAL)
			return 1; // more than 1 group
		else
			return -1; // other error
	}

	if (rc == 1 && groups[0] != gid)
		return 1;

	return 0;
}

static char* ngx_dropcap_init_conf(ngx_cycle_t *cycle, void *conf) { // NOSONAR
	ngx_int_t rc = ngx_dropcap_has_supplementary();
	if (rc == -1) {
		return NGX_CONF_ERROR ;
	} else if (rc) {
		struct __user_cap_header_struct cap_header = {0};
		cap_header.version = _LINUX_CAPABILITY_VERSION_3;

		struct __user_cap_data_struct cap_data[_LINUX_CAPABILITY_U32S_3];
		ngx_memzero(cap_data, sizeof(cap_data));

		if (syscall(SYS_capget, &cap_header, cap_data)) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, errno, "dropcap: failed to read capabilities for master");
			return NGX_CONF_ERROR ;
		}

		if ((cap_data[0].effective & CAP_TO_MASK(CAP_SETGID)) == 0) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, 0,
					"dropcap: master has supplementary groups but no CAP_SETGID capability");
			return NGX_CONF_ERROR ;
		}
	}
	return NGX_OK;
}

static ngx_int_t ngx_dropcap_init_process(ngx_cycle_t *cycle) { // NOSONAR
	// drop groups
	ngx_int_t rc = ngx_dropcap_has_supplementary();
	if (rc == -1) {
		return NGX_ERROR;
	} else if (rc) {
		if (setgroups(0, NULL)) {
			ngx_log_error(NGX_LOG_EMERG, cycle->log, errno, "dropcap: failed to set groups");
			return NGX_ERROR;
		}
	}

	// drop caps
	struct __user_cap_header_struct cap_header = {0};
	cap_header.version = _LINUX_CAPABILITY_VERSION_3;

	struct __user_cap_data_struct cap_data[_LINUX_CAPABILITY_U32S_3];
	ngx_memzero(cap_data, sizeof(cap_data));

	if (syscall(SYS_capset, &cap_header, cap_data)) {
		ngx_log_error(NGX_LOG_EMERG, cycle->log, errno, "dropcap: failed to drop capabilities for worker", CAP_SETPCAP);
		return NGX_ERROR;
	}

	return NGX_OK;
}

// @formatter:off
static ngx_core_module_t ngx_dropcap_module_ctx = {
	ngx_string("dropcap"),
	NULL,
	ngx_dropcap_init_conf
};

static ngx_command_t  ngx_dropcap_commands[] = {ngx_null_command};

ngx_module_t ngx_dropcap_module = {
	NGX_MODULE_V1,
	&ngx_dropcap_module_ctx,
	ngx_dropcap_commands,
	NGX_CORE_MODULE,
	NULL,
	NULL,
	ngx_dropcap_init_process,
	NULL,
	NULL,
	NULL,
	NULL,
	NGX_MODULE_V1_PADDING
};
// @formatter:on
