$!
$! Generated by genbuild.py
$!
$ set default swig_root:[vms]
$
$ @swig_root:[vms]build_init
$ @swig_root:[vms.scripts]compil_cparse.com
$ @swig_root:[vms.scripts]compil_doh.com
$ @swig_root:[vms.scripts]compil_modules1_1.com
$ @swig_root:[vms.scripts]compil_preprocessor.com
$ @swig_root:[vms.scripts]compil_swig.com
$
$ set default swig_root:[vms]
$
$ @swig_root:[vms]build_end
