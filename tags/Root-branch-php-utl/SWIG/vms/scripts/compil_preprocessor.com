$!
$! Generated by genbuild.py
$!
$ libname = "swig_root:[vms.o_alpha]swig.olb"
$
$ set default SWIG_ROOT:[SOURCE.PREPROCESSOR]
$
$ idir :=  swig_root:[source.swig]
$ idir = idir + ",swig_root:[source.doh.include]"
$ idir = idir + ",swig_root:[source.include]"
$ idir = idir + ",swig_root:[source.preprocessor]"
$
$ iflags = "/include=(''idir', sys$disk:[])"
$ oflags = "/object=swig_root:[vms.o_alpha]
$ cflags = "''oflags'''iflags'''dflags'"
$ cxxflags = "''oflags'''iflags'''dflags'"
$
$ call make swig_root:[vms.o_alpha]cpp.obj -
	"cc ''cflags'" cpp.c
$ call make swig_root:[vms.o_alpha]expr.obj -
	"cc ''cflags'" expr.c
$ exit
$!
$!
$MAKE: SUBROUTINE   !SUBROUTINE TO CHECK DEPENDENCIES
$ V = 'F$Verify(0)
$! P1 = What we are trying to make
$! P2 = Command to make it
$! P3 = Source file
$! P4 - P8  What it depends on
$
$ modname = f$parse(p3,,,"name")
$ set noon
$ set message/nofacility/noident/noseverity/notext
$ libr/lis=swig_root:[vms]swiglib.tmp/full/width=132/only='modname' 'libname'
$ set message/facility/ident/severity/text
$ on error then exit
$ open/read swigtmp swig_root:[vms]swiglib.tmp
$! skip header
$ read swigtmp r
$ read swigtmp r
$ read swigtmp r
$ read swigtmp r
$ read swigtmp r
$ read swigtmp r
$ read swigtmp r
$ read swigtmp r
$ read swigtmp r
$!
$
$ read/end=module_not_found swigtmp r
$ modfound = 1
$ Time = f$cvtime(f$extract(49, 20, r))
$ goto end_search_module
$ module_not_found:
$ modfound = 0
$
$ end_search_module:
$ close swigtmp
$ delete swig_root:[vms]swiglib.tmp;*
$
$ if modfound .eq. 0 then $ goto Makeit
$
$! Time = F$CvTime(F$File(P1,"RDT"))
$arg=3
$Loop:
$       Argument = P'arg
$       If Argument .Eqs. "" Then Goto Exit
$       El=0
$Loop2:
$       File = F$Element(El," ",Argument)
$       If File .Eqs. " " Then Goto Endl
$       AFile = ""
$Loop3:
$       OFile = AFile
$       AFile = F$Search(File)
$       If AFile .Eqs. "" .Or. AFile .Eqs. OFile Then Goto NextEl
$       If F$CvTime(F$File(AFile,"RDT")) .Ges. Time Then Goto Makeit
$       Goto Loop3
$NextEL:
$       El = El + 1
$       Goto Loop2
$EndL:
$ arg=arg+1
$ If arg .Le. 8 Then Goto Loop
$ Goto Exit
$
$Makeit:
$ VV=F$VERIFY(1)
$ 'P2' 'P3'
$ VV='F$Verify(VV)
$Exit:
$ If V Then Set Verify
$ENDSUBROUTINE
