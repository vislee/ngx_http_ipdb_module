#!/usr/bin/perl

# (C) vislee

# Tests for http ipdb module.

###############################################################################

use warnings;
use strict;

use Test::More;

use Socket qw/ CRLF /;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx;

###############################################################################

select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new();

$t->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

# load_module /tmp/nginx/modules/ngx_http_ipdb_module.so;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    ipdb ./ipiptest.ipdb;
    ipdb_language CN;
    ipdb_proxy 127.0.0.1;
    ipdb_proxy 192.168.0.1/24;
    ipdb_proxy 255.255.255.255;
    ipdb_proxy_recursive on;

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        #set_real_ip_from 127.0.0.1;
        #real_ip_header X-Forwarded-For;

        location /test/ {
            return 200 testipdb;
        }

        location /country/ {
            return 200 $ipdb_country_name;
        }

        location /ipv6/country/ {
            return 200 null$ipdb_country_name;
        }

        location /region/ {
            return 200 $ipdb_region_name;
        }

        location /city/ {
            return 200 $ipdb_city_name;
        }

        location /isp/ {
            return 200 null$ipdb_isp_domain;
        }

        location /spec/country {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 $ipdb_country_name;
        }

        location /spec/region {
            ipdb_specifies_addr $http_addr;
            return 200 $ipdb_region_name;
        }

        location /spec/city/ {
            ipdb_specifies_addr $http_addr;
            return 200 $ipdb_city_name;

            location /spec/city/args {
                ipdb_specifies_addr $arg_addr;
                return 200 $ipdb_city_name;
            }

        }
    }

}

EOF

$t->try_run('no ipdb')->plan(12);

###############################################################################
like(http(<<EOF), qr/testipdb/, 'ipdb test');
GET /test/ HTTP/1.0
Host: localhost
X-Forwarded-For: 192.168.0.1,36.102.4.81

EOF

like(http(<<EOF), qr/中国/, 'ipdb country');
GET /country/ HTTP/1.0
Host: localhost
X-Forwarded-For: 36.102.4.81,192.168.0.1

EOF


like(http(<<EOF), qr/null$/, 'ipdb country');
GET /ipv6/country/ HTTP/1.0
Host: localhost
X-Forwarded-For: a80::7cfc:c767:aded:c04e

EOF


like(http(<<EOF), qr/局域网/, 'ipdb country proxy');
GET /country/ HTTP/1.0
Host: localhost
X-Forwarded-For: 36.102.4.81,192.168.1.2

EOF


like(http(<<EOF), qr/本机地址/, 'ipdb country proxy');
GET /country/ HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/内蒙古/, 'ipdb region');
GET /region/ HTTP/1.0
Host: localhost
X-Forwarded-For: 36.102.4.81,192.168.0.1

EOF


like(http(<<EOF), qr/呼和浩特/, 'ipdb city');
GET /city/ HTTP/1.0
Host: localhost
X-Forwarded-For: 36.102.4.81,192.168.0.1

EOF


like(http(<<EOF), qr/null$/, 'ipdb isp');
GET /isp/ HTTP/1.0
Host: localhost
X-Forwarded-For: 36.102.4.81,192.168.0.1

EOF


like(http(<<EOF), qr/中国/, 'ipdb spec country');
GET /spec/country?addr=36.102.4.81 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/内蒙古/, 'ipdb spec region');
GET /spec/region HTTP/1.0
Host: localhost
addr: 36.102.4.81

EOF


like(http(<<EOF), qr/呼和浩特/, 'ipdb spec region');
GET /spec/city/ HTTP/1.0
Host: localhost
addr: 36.102.4.81

EOF


like(http(<<EOF), qr/北京/, 'ipdb spec region');
GET /spec/city/args?addr=119.75.210.2 HTTP/1.0
Host: localhost
addr: 36.102.4.81

EOF
