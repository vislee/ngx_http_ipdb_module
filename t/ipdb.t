#!/usr/bin/perl

# (C) vislee

# Tests for http waf module.

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

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    ipdb ./ipiptest.ipdb;
    ipdb_language CN;
    ipdb_proxy 127.0.0.1;
    ipdb_proxy 192.168.0.1;
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

        location /region/ {
            return 200 $ipdb_region_name;
        }

        location /city/ {
            return 200 $ipdb_city_name;
        }
    }

}

EOF

$t->try_run('no ipdb')->plan(6);

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


like(http(<<EOF), qr/局域网/, 'ipdb country proxy');
GET /country/ HTTP/1.0
Host: localhost
X-Forwarded-For: 36.102.4.81,192.168.0.2

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


