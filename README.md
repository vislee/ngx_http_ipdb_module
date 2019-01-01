Name
====

ngx_http_ipdb_module - creates variables with values depending on the client IP address, using the precompiled [ipip.net](https://www.ipip.net) ipdb.

Table of Contents
=================
* [Name](#name)
* [Status](#status)
* [Install](#install)
* [Example Configuration](#example-configuration)
* [Directives](#directives)
    * [ipdb](#ipdb)
    * [ipdb_language](#ipdb_language)
* [Variable](#variable)
    * [ipdb_region_name](#ipdb_region_name)
    * [ipdb_city_name](#ipdb_city_name)
* [TODO](#todo)
* [Author](#author)
* [Copyright and License](#copyright-and-license)


Status
======
The module is currently in active development.

[Back to TOC](#table-of-contents)

Example Configuration
====================

```nginx
http {
    include       mime.types;
    default_type  application/octet-stream;

    ......

    ipdb /tmp/nginx/conf/ipiptest.ipdb;
    ipdb_language CN;

    server {
        listen       8090;
        server_name  localhost;

        ......

        set_real_ip_from 127.0.0.1;
        real_ip_header X-Forwarded-For;

        location / {
            return 200 $ipdb_city_name;
        }
    }
}

```

[Back to TOC](#table-of-contents)

TODO
==========

 + add variable
     * ipdb_country_code
     * ipdb_country_name
     * ipdb_org
     * ipdb_latitude
     * ipdb_longitude
 + from header ip
 + from args ip

[Back to TOC](#table-of-contents)

Directives
==========

ipdb
----

ipdb_language
-------------

[Back to TOC](#table-of-contents)

Variable
========

ipdb_region_name
----------------

ipdb_city_name
--------------

[Back to TOC](#table-of-contents)

Author
======

wenqiang li(vislee)

[Back to TOC](#table-of-contents)

Copyright and License
=====================

This module is licensed under the GPL license.

Copyright (C) 2018-2019, by vislee.

All rights reserved.

[Back to TOC](#table-of-contents)
