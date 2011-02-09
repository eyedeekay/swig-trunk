# This file was automatically generated by SWIG (http://www.swig.org).
# Version 2.0.2
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

from sys import version_info
if version_info >= (3,0,0):
    new_instancemethod = lambda func, inst, cls: _Simple_optimized.SWIG_PyInstanceMethod_New(func)
else:
    from new import instancemethod as new_instancemethod
if version_info >= (2,6,0):
    def swig_import_helper():
        from os.path import dirname
        import imp
        fp = None
        try:
            fp, pathname, description = imp.find_module('_Simple_optimized', [dirname(__file__)])
        except ImportError:
            import _Simple_optimized
            return _Simple_optimized
        if fp is not None:
            try:
                _mod = imp.load_module('_Simple_optimized', fp, pathname, description)
            finally:
                fp.close()
            return _mod
    _Simple_optimized = swig_import_helper()
    del swig_import_helper
else:
    import _Simple_optimized
del version_info
try:
    _swig_property = property
except NameError:
    pass # Python < 2.2 doesn't have 'property'.
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "thisown"): return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'SwigPyObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    if (name == "thisown"): return self.this.own()
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError(name)

def _swig_repr(self):
    try: strthis = "proxy of " + self.this.__repr__()
    except: strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

try:
    _object = object
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0


def _swig_setattr_nondynamic_method(set):
    def set_attr(self,name,value):
        if (name == "thisown"): return self.this.own(value)
        if hasattr(self,name) or (name == "this"):
            set(self,name,value)
        else:
            raise AttributeError("You cannot add attributes to %s" % self)
    return set_attr


class A(object):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.A_swiginit(self,_Simple_optimized.new_A())
    __swig_destroy__ = _Simple_optimized.delete_A
A.func = new_instancemethod(_Simple_optimized.A_func,None,A)
A_swigregister = _Simple_optimized.A_swigregister
A_swigregister(A)

class B(A):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.B_swiginit(self,_Simple_optimized.new_B())
    __swig_destroy__ = _Simple_optimized.delete_B
B_swigregister = _Simple_optimized.B_swigregister
B_swigregister(B)

class C(B):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.C_swiginit(self,_Simple_optimized.new_C())
    __swig_destroy__ = _Simple_optimized.delete_C
C_swigregister = _Simple_optimized.C_swigregister
C_swigregister(C)

class D(C):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.D_swiginit(self,_Simple_optimized.new_D())
    __swig_destroy__ = _Simple_optimized.delete_D
D_swigregister = _Simple_optimized.D_swigregister
D_swigregister(D)

class E(D):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.E_swiginit(self,_Simple_optimized.new_E())
    __swig_destroy__ = _Simple_optimized.delete_E
E_swigregister = _Simple_optimized.E_swigregister
E_swigregister(E)

class F(E):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.F_swiginit(self,_Simple_optimized.new_F())
    __swig_destroy__ = _Simple_optimized.delete_F
F_swigregister = _Simple_optimized.F_swigregister
F_swigregister(F)

class G(F):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.G_swiginit(self,_Simple_optimized.new_G())
    __swig_destroy__ = _Simple_optimized.delete_G
G_swigregister = _Simple_optimized.G_swigregister
G_swigregister(G)

class H(G):
    thisown = _swig_property(lambda x: x.this.own(), lambda x, v: x.this.own(v), doc='The membership flag')
    __repr__ = _swig_repr
    def __init__(self): 
        _Simple_optimized.H_swiginit(self,_Simple_optimized.new_H())
    __swig_destroy__ = _Simple_optimized.delete_H
H_swigregister = _Simple_optimized.H_swigregister
H_swigregister(H)



