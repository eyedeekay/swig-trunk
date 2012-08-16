
import doxygen_tricky_constructs.*;
import com.sun.javadoc.*;
import java.util.HashMap;

public class doxygen_tricky_constructs_runme {
  static {
    try {
      System.loadLibrary("doxygen_tricky_constructs");
    } catch (UnsatisfiedLinkError e) {
      System.err.println("Native code library failed to load. See the chapter on Dynamic Linking Problems in the SWIG Java documentation for help.\n" + e);
      System.exit(1);
    }
  }
  
  public static void main(String argv[]) 
  {
    /*
      Here we are using internal javadoc tool, it accepts the name of the class as paramterer,
      and calls the start() method of that class with parsed information.
    */
	commentParser parser = new commentParser();
    com.sun.tools.javadoc.Main.execute("doxygen_tricky_constructs runtime test",
	"commentParser", new String[]{"-quiet", "doxygen_tricky_constructs"});

    HashMap<String, String> wantedComments = new HashMap<String, String>();
    
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.getConnection()",
    		" \n" +
    		" \n" +
    		" This class manages connection. \n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.getAddress(doxygen_tricky_constructs.SWIGTYPE_p_int, int)",
    		" Returns address of file line. \n" +
    		" \n" +
    		" @param fileName name of the file, where the source line is located \n" +
    		" @param line line number \n" +
    		" {@link Connection::getId()  }<br> \n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.getG_zipCode()",
    		" Tag endlink must be recognized also when it is the last token \n" +
    		" in the commment. \n" +
    		" \n" +
    		" {@link Connection::getId()  }<br> \n" +
    		" {@link debugIdeTraceProfilerCoverageSample.py Python example.  }\n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.setG_zipCode(int)",
    		" Tag endlink must be recognized also when it is the last token \n" +
    		" in the commment. \n" +
    		" \n" +
    		" {@link Connection::getId()  }<br> \n" +
    		" {@link debugIdeTraceProfilerCoverageSample.py Python example.  }\n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.getG_counter()",
    		" Tag endlink must be recognized also when followed by nonspace charater. \n" +
    		" \n" +
    		" {@link Connection::getId()  }<br> \n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.waitTime(int)",
    		" Determines how long the <code>isystem.connect</code> should wait for running \n" +
    		" instances to respond. Only one of <code>lfWaitXXX</code> flags from IConnect::ELaunchFlags \n" +
    		" may be specified. \n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.CConnectionConfig",
    		" This class contains information for connection to winIDEA. Its methods \n" +
    		" return reference to self, so we can use it like this: \n" +
    		" <pre> \n" +
    		" CConnectionConfig config = new CConnectionConfig(); \n" +
    		" config.discoveryPort(5534).dllPath(\"C: \\ yWinIDEA \\ onnect.dll\").id(\"main\"); \n" +
    		" </pre> \n" +
    		" \n" +
    		" All parameters are optional. Set only what is required, default values are \n" +
    		" used for unspecified parameters. \n" +
    		" <p> \n" +
    		" \n" +
    		" {@link advancedWinIDEALaunching.py Python example.  }<br> \n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.getAddress(doxygen_tricky_constructs.SWIGTYPE_p_int, int, boolean)",
    		" Returns address of file line. \n" +
    		" \n" +
    		" @param fileName name of the file, where the source line is located \n" +
    		" @param line line number \n" +
    		" @param isGetSize if set, for every object location both address and size are returned \n" +
    		" \n" +
    		" {@link Connection::getId()  }<br> \n" +
    		" \n" +
    		"");
    wantedComments.put("doxygen_tricky_constructs.doxygen_tricky_constructs.setG_counter(char)",
    		" Tag endlink must be recognized also when followed by nonspace charater. \n" +
    		" \n" +
    		" {@link Connection::getId()  }<br> \n" +
    		" \n" +
    		"");
    
    // and ask the parser to check comments for us
    System.exit(parser.check(wantedComments));
  }
}