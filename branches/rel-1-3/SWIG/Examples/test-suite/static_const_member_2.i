%module static_const_member_2


%inline %{ 
 namespace oss 
 {   
   namespace modules
   {
     struct CavityPackFlags 
     {
       typedef unsigned int viewflags;
       static const viewflags forward_field  = 1 << 0;
       static const viewflags backward_field = 1 << 1;
       static const viewflags cavity_flags;
       static viewflags flags;

     };     

     template <class T>
     struct Test : CavityPackFlags
     {
       enum {LeftIndex, RightIndex};
       static const viewflags current_profile  = 1 << 2;
     };
   }
 }

%} 

%{
  using oss::modules::CavityPackFlags;
  
  const CavityPackFlags::viewflags 
    CavityPackFlags::cavity_flags = 
    CavityPackFlags::forward_field | CavityPackFlags::backward_field;

  CavityPackFlags::viewflags 
    CavityPackFlags::flags = 0;

%}

%template(Test_int) oss::modules::Test<int>;

