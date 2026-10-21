#include <grp.h>
#include <linux/capability.h>
#include <ngx_conf_file.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_log.h>
#include <ngx_module.h>
#include <ngx_string.h>
#include <string.h>
#include <syscall.h>
#include <unistd.h>

static ngx_int_t ngx_dropcap_init_process(ngx_cycle_t *cycle) { // NOSONAR
	if (setgroups(0, NULL)) {
		ngx_log_error(NGX_LOG_CRIT, cycle->log, errno, "failed to set groups");
		return NGX_ERROR;
	}

	struct __user_cap_header_struct cap_header = {0};
	cap_header.version = _LINUX_CAPABILITY_VERSION_3;

	struct __user_cap_data_struct cap_data[_LINUX_CAPABILITY_U32S_3];
	memset(cap_data, 0, sizeof(cap_data));

	if (syscall(SYS_capset, &cap_header, cap_data)) {
		ngx_log_error(NGX_LOG_CRIT, cycle->log, errno, "failed to drop capabilities for worker", CAP_SETPCAP);
		return NGX_ERROR;
	}

	return NGX_OK;
}

// @formatter:off
static ngx_core_module_t ngx_dropcap_module_ctx = {
	ngx_string("dropcap"),
	NULL,
	NULL
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
