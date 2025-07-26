$ cc simple_stomp.c
$ cc vms_simple_stomp.c
$ if f$search("[-.pcommon]common.pas") .nes. "" .and. -
     f$search("[-.pcommon]common.pen") .nes. "" .and. -
     f$search("[-.pcommon]common.obj") .nes. ""
$ then
$    copy/log [-.pcommon]common.pen *.*
$    copy/log [-.pcommon]common.obj *.*
$ else
$    pas/env common
$ endif
$ pas/env pstomp
$ exit
