/*

  Create a file with this name, 'fragments.i', in your working
  directory and add all the %fragments you want to take precedence
  over the ones defined by default by swig.

  For example, if you add:
  
  %fragment(SWIG_AsVal_frag(int),"header") {
   SWIGSTATICINLINE(int)
   SWIG_AsVal(int)(PyObject *obj, int *val)
   { 
     <your code here>;
   }
  }
  
  this will replace the code used to retreive an integer value for all
  the typemaps that need it, including:
  
    int, std::vector<int>, std::list<std::pair<int,int> >, etc.

    
*/
