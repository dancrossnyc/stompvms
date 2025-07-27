[inherit('common')]
module pstomp;

type
   stomp_ctx = array [1..4096] of integer;

[external('VMS_SIMPLE_STOMP_DEBUG')]
procedure stomp_debug(%REF debug : integer); extern;

[external('VMS_SIMPLE_STOMP_INIT')]
function stomp_init(%REF ctx : stomp_ctx;
                    %STDESCR host : packed array [l1..u1:integer] of char;
                    %REF port : integer;
                    %STDESCR client_id: packed array [l2..u2:integer] of char,
                    %STDESCR username: packed array [l3..u3:integer] of char,
                    %STDESCR password: packed array [l4..u4:integer] of char
                  ) : integer; extern;

[external('VMS_SIMPLE_STOMP_WRITE')]
function stomp_write(%REF ctx : stomp_ctx;
                     %STDESCR qname : packed array [l1..u1:integer] of char;
                     %STDESCR msg : packed array [l2..u2:integer] of char) : integer; extern;

[external('VMS_SIMPLE_STOMP_SUB')]
function stomp_sub(%REF ctx : stomp_ctx;
                   %STDESCR qname : packed array [l1..u1:integer] of char) : integer; extern;

[external('VMS_SIMPLE_STOMP_UNSUB')]
function stomp_unsub(%REF ctx : stomp_ctx;
                     %STDESCR qname : packed array [l1..u1:integer] of char) : integer; extern;

[external('VMS_SIMPLE_STOMP_READONE')]
function stomp_readone(%REF ctx : stomp_ctx;
                       %STDESCR msg : packed array [l2..u2:integer] of char;
                       %REF msglen : integer) : integer; extern;

[external('VMS_SIMPLE_STOMP_READ')]
function stomp_read(%REF ctx : stomp_ctx;
                    %STDESCR qname : packed array [l1..u1:integer] of char;
                    %STDESCR msg : packed array [l2..u2:integer] of char;
                    %REF msglen : integer) : integer; extern;

[external('VMS_SIMPLE_STOMP_CLOSE')]
function stomp_close(%REF ctx : stomp_ctx) : integer; extern;

end.
