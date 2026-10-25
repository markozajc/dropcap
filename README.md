# dropcap
Nginx module to drop supplementary groups and capabilities when the master process isn't running as root.

On most distributions, the Nginx process is by default started as root and only drops privileges in worker processes. This can be reconfigured, but any extra privileges granted to the master process, such as `CAP_NET_BIND_SERVICE` to bind low ports and supplementary groups to read TLS keys will also be granted to workers.

## Installation

### Debian
Ready-made packages for Debian stable amd64 are available at https://files.zajc.tel/public/builds/dropcap. If you get an error about a missing `nginx-abi-*` package when installing, you're have an incompatible version of Nginx installed.

Users of other releases or architectures can very easily compile the package with the instructions below.

1. Install build dependencies

	```bash
	apt install git nginx-dev dpkg-dev
	```

2. Clone the repository

	```bash
	git clone https://git.zajc.tel/dropcap.git && cd libnginx-mod-dropcap
	```

3. Compile the package

	```bash
	dpkg-buildpackage --no-sign
	```

4. Packages will be created in the parent directory.

dropcap is built against a certain version of the Nginx ABI - it depends on `nginx-abi-*`, for example `nginx-abi-1.26.3-1`, which is provided by the `nginx` package depending on its version. Some Nginx updates will break compatibility, meaning you have to recompile dropcap before Nginx can be upgraded.

### Other
You will need to compile and install the module by hand.

1. Install build dependencies for Nginx. dropcap does not require anything more than that.
2. Download the source of the Nginx binaries you're using. If you're getting Nginx from your distribution, make sure you use the same source files (including patches and whatnot), otherwise you will likely into issues when loading the module because Nginx has no ABI stability. The official source distribution is at `https://nginx.org/download/nginx-<version>.tar.gz`
3. Clone the module repository from `https://git.zajc.tel/dropcap.git` or download and extract the source tarball from `https://files.zajc.tel/public/builds/dropcap/<version>/source.tar.xz` and `cd` into it.
4. (optional) Adjust build configuration in `src/config.h`.
5. Build the module binary with
	```bash
	NGINX_PATH=/path/to/nginx/source tools/build.sh
	```
6. The script will tell you where the `ngx_dropcap_module.so` file was created. Copy it to the Nginx modules directory - this is usually `/usr/lib/nginx/modules`, but you should consult the `--modules-path` build flag that your Nginx binary was compiled with (`nginx -V` to see all flags) for the exact path.
7. Create the module config file.
	```bash
	echo "load_module modules/ngx_dropcap_module.so;" | sudo tee /etc/nginx/modules-available/mod-http-dropcap.conf
	sudo ln -s ../mod-http-dropcap.conf /etc/nginx/modules-enabled/40-mod-http-dropcap.conf
	```
8. Restart Nginx.

#### Portability
Because Nginx doesn't link libcap or libcap-ng, this module manages capabilities by sending syscalls manually. This should work across Linux versions (V3 syscall interface was added in 2.6.26 and is forward compatible), but will not work on other POSIX systems.

Windows has entirely different privilege management, so this module won't work.

## Usage
This module has no configuration. However, there are some considerations when configuring the systemd service for Nginx. Sample configuration for running Nginx as a non-root user on Debian:

```
# /etc/nginx/nginx.conf
pid /run/nginx/nginx.pid;
```

```
# /etc/systemd/system/nginx.service.d/override.conf
[Service]
User=www-data
RuntimeDirectory=nginx
PIDFile=/run/nginx/nginx.pid
ExecStop=-/sbin/start-stop-daemon --quiet --stop --retry QUIT/5 --pidfile /run/nginx/nginx.pid

SupplementaryGroups=ssl-cert
AmbientCapabilities=CAP_NET_BIND_SERVICE CAP_SETGID
CapabilityBoundingSet=CAP_NET_BIND_SERVICE CAP_SETGID
```

- Because the Nginx master process can't run workers as a different user when it's not running as root, it should run as the same user as workers (on Debian this is `www-data` by default). You should also remove the `user` directive from `nginx.conf`, otherwise you'll get a warning on every startup.
- If supplementary groups are set, `CAP_SETGID` must also be granted, otherwise workers will not be able to drop privileges.
- Nginx's pidfile must be in a location where it can be created by the master process user. Since `/run` is root-writable on Debian, the pidfile can instead be moved to `/run/nginx/nginx.pid`, thanks to systemd's `RuntimeDirectory`.
- TLS keys must be readable by a supplementary group, which may require extra setup for ACME clients like Certbot. You can also specify multiple supplementary groups, separated by spaces.

## Mirrors
- https://git.zajc.tel/dropcap.git
- https://github.com/markozajc/dropcap
