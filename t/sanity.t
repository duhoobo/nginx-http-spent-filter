# vi:filetype=

use lib 'lib';
use Test::Nginx::Socket;

repeat_each(2);

plan tests => repeat_each() * 18;

#master_on();
#workers(2);
log_level("warn");
no_diff;

run_tests();

__DATA__


=== TEST 1: direct request with spent default off
--- config
    location /foo {
        echo hi;
    }
--- request
    GET /foo
--- response_headers
! X-Spent
--- response_body
hi
--- error_code: 200


=== TEST 2: direct request with spent off
--- config
    location /foo {
        echo hi;
        spent off;
    }
--- request
    GET /foo
--- response_headers
! X-Spent
--- response_body
hi
--- error_code: 200


=== TEST 3: direct request with spent in active mode
--- config
    location /foo {
        echo hi;
        spent active;
        spent_header X-Spenty;
        spent_prefix y;
    }
--- request
    GET /foo
--- response_headers_like
X-Spenty: y\d{3}\.\d{3};.*y\d{3}\.\d{3}$
--- response_body
hi
--- error_code: 200


=== TEST 4: direct request with spent in passive mode
--- config
    location /foo {
        echo hi;
        spent passive;
    }
--- request
    GET /foo
--- response_headers
! X-Spent
--- response_body
hi
--- error_code: 200


=== TEST 5: fastcgi request with spent off
--- config
    location /xspent.php {
        fastcgi_pass 127.0.0.1:9000;

        fastcgi_param  SCRIPT_FILENAME    /tmp$fastcgi_script_name;
        fastcgi_param  QUERY_STRING       $query_string;
        fastcgi_param  REQUEST_METHOD     $request_method;
        fastcgi_param  CONTENT_TYPE       $content_type;
        fastcgi_param  CONTENT_LENGTH     $content_length;
        
        fastcgi_param  SCRIPT_NAME        $fastcgi_script_name;
        fastcgi_param  REQUEST_URI        $request_uri;
        fastcgi_param  DOCUMENT_URI       $document_uri;
        fastcgi_param  DOCUMENT_ROOT      $document_root;
        fastcgi_param  SERVER_PROTOCOL    $server_protocol;
        fastcgi_param  HTTPS              $https if_not_empty;
        
        fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;
        fastcgi_param  SERVER_SOFTWARE    nginx/$nginx_version;
        
        fastcgi_param  REMOTE_ADDR        $remote_addr;
        fastcgi_param  REMOTE_PORT        $remote_port;
        fastcgi_param  SERVER_ADDR        $server_addr;
        fastcgi_param  SERVER_PORT        $server_port;
        fastcgi_param  SERVER_NAME        $server_name;
        
        fastcgi_param  REDIRECT_STATUS    200;
    }
--- request
    GET /xspent.php
--- respone_headers_like
X-Spent: d\d{3}\.\d{3};.*d\d{3}\.\d{3}
--- response_body
hi
--- error_code: 200


=== TEST 6: fastcgi request with spent passive
--- config
    location /xspent.php {
        spent passive;
        spent_header X-Spent;
        spent_prefix y;

        fastcgi_pass 127.0.0.1:9000;

        fastcgi_param  SCRIPT_FILENAME    /tmp$fastcgi_script_name;
        fastcgi_param  QUERY_STRING       $query_string;
        fastcgi_param  REQUEST_METHOD     $request_method;
        fastcgi_param  CONTENT_TYPE       $content_type;
        fastcgi_param  CONTENT_LENGTH     $content_length;
        
        fastcgi_param  SCRIPT_NAME        $fastcgi_script_name;
        fastcgi_param  REQUEST_URI        $request_uri;
        fastcgi_param  DOCUMENT_URI       $document_uri;
        fastcgi_param  DOCUMENT_ROOT      $document_root;
        fastcgi_param  SERVER_PROTOCOL    $server_protocol;
        fastcgi_param  HTTPS              $https if_not_empty;
        
        fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;
        fastcgi_param  SERVER_SOFTWARE    nginx/$nginx_version;
        
        fastcgi_param  REMOTE_ADDR        $remote_addr;
        fastcgi_param  REMOTE_PORT        $remote_port;
        fastcgi_param  SERVER_ADDR        $server_addr;
        fastcgi_param  SERVER_PORT        $server_port;
        fastcgi_param  SERVER_NAME        $server_name;
        
        fastcgi_param  REDIRECT_STATUS    200;
    }
--- request
    GET /xspent.php
--- respone_headers_like
X-Spent: y\d{3}\.\d{3};.*y\d{3}\.\d{3}
--- response_body
hi
--- error_code: 200


=== TEST 7: fastcgi request with spent active 
--- config
    location /xspent.php {
        spent active;
        spent_header X-Spent;
        spent_prefix y;

        fastcgi_pass 127.0.0.1:9000;

        fastcgi_param  SCRIPT_FILENAME    /tmp$fastcgi_script_name;
        fastcgi_param  QUERY_STRING       $query_string;
        fastcgi_param  REQUEST_METHOD     $request_method;
        fastcgi_param  CONTENT_TYPE       $content_type;
        fastcgi_param  CONTENT_LENGTH     $content_length;
        
        fastcgi_param  SCRIPT_NAME        $fastcgi_script_name;
        fastcgi_param  REQUEST_URI        $request_uri;
        fastcgi_param  DOCUMENT_URI       $document_uri;
        fastcgi_param  DOCUMENT_ROOT      $document_root;
        fastcgi_param  SERVER_PROTOCOL    $server_protocol;
        fastcgi_param  HTTPS              $https if_not_empty;
        
        fastcgi_param  GATEWAY_INTERFACE  CGI/1.1;
        fastcgi_param  SERVER_SOFTWARE    nginx/$nginx_version;
        
        fastcgi_param  REMOTE_ADDR        $remote_addr;
        fastcgi_param  REMOTE_PORT        $remote_port;
        fastcgi_param  SERVER_ADDR        $server_addr;
        fastcgi_param  SERVER_PORT        $server_port;
        fastcgi_param  SERVER_NAME        $server_name;
        
        fastcgi_param  REDIRECT_STATUS    200;
    }
--- request
    GET /xspent.php
--- respone_headers_like
X-Spent: y\d{3}\.\d{3};.*y\d{3}\.\d{3}
--- response_body
hi
--- error_code: 200

