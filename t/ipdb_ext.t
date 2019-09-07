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

    ipdb ./ipip.ipdb;
    ipdb_language CN;

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        location /spec/owner_domain {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 $ipdb_owner_domain;
        }

        location /spec/xitude {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_latitude,$ipdb_longitude";
        }

        location /spec/timezone {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_timezone";
        }

        location /spec/utc_offset {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_utc_offset";
        }

        location /spec/china_admin_code {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_china_admin_code";
        }

        location /spec/idd_code {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_idd_code";
        }

        location /spec/country_code {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_country_code";
        }

        location /spec/continent_code {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_continent_code";
        }

        location /spec/idc {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_idc";
        }

        location /spec/currency_name {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_currency_name";
        }

        location /spec/country_code3 {
            ipdb_specifies_addr $arg_addr;
            ipdb_language CN;
            return 200 "$ipdb_country_code3";
        }

    }

}

EOF

$t->try_run('no ipdb')->plan(11);

###############################################################################
like(http(<<EOF), qr/清华大学/, 'ipdb spec owner_domain');
GET /spec/owner_domain?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/39.[0-9]*,116.[0-9]*/, 'ipdb spec latitude longitude');
GET /spec/xitude?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/Asia\/Shanghai/, 'ipdb spec timezone');
GET /spec/timezone?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/UTC\+8/, 'ipdb spec utc_offset');
GET /spec/utc_offset?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/110000/, 'ipdb spec china_admin_code');
GET /spec/china_admin_code?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/86/, 'ipdb spec idd_code');
GET /spec/idd_code?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/CN/, 'ipdb spec country_code');
GET /spec/country_code?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr/AP/, 'ipdb spec continent_code');
GET /spec/continent_code?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr//, 'ipdb spec idc');
GET /spec/idc?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr//, 'ipdb spec currency_name');
GET /spec/currency_name?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


like(http(<<EOF), qr//, 'ipdb spec country_code3');
GET /spec/country_code3?addr=166.111.4.105 HTTP/1.0
Host: localhost

EOF


