module common(input, output);

[external('decc$free')]
procedure free_c_str(ptr : c_str_t); external;

type
   pstr = varying [32000] of char;
   word = [word] 0..32767;
   byte = [byte] -128..127;

function fix(s : pstr) : pstr;

begin
   fix := substr(s, 1, s.length);
end;

end.
