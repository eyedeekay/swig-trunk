// Tests typename qualification and constant resolution in default
// template arguments.  Another Marcelo special.. :-).

%module template_default_qualify

%warnfilter(801) etraits; /* Ruby, wrong class name */

%inline %{
  namespace oss
  {
 
    enum Polarization { UnaryPolarization, BinaryPolarization };
 
    template <Polarization P>
    struct Interface
    {
    };

    namespace modules
    {
 
      template <class traits, class base = Interface<traits::pmode> >
 // *** problem here ****
      struct Module : base
      {
      };
    }
  }
  struct etraits
  {
    static const oss::Polarization pmode = oss::UnaryPolarization;
  };

%}
 
namespace oss
{	
  %template(Interface_UP) Interface<UnaryPolarization>;
  namespace modules
  {
    %template(Module_etraits) Module<etraits>;
  }
}
 
%inline %{
  namespace oss
  {
    namespace modules
    {
      struct HModule1 : Module<etraits>
      {
      };
    }
  }
%}
                                        
