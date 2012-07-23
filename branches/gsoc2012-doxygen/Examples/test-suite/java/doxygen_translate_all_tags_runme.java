
import doxygen_translate_all_tags.*;
import com.sun.javadoc.*;
import java.util.HashMap;

public class doxygen_translate_all_tags_runme {
  static {
    try {
      System.loadLibrary("doxygen_translate_all_tags");
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
    com.sun.tools.javadoc.Main.execute("doxygen_translate_all_tags runtime test",
	"commentParser", new String[]{"-quiet", "doxygen_translate_all_tags"});

    HashMap<String, String> wantedComments = new HashMap<String, String>();
    
    wantedComments.put("doxygen_translate_all_tags.doxygen_translate_all_tags.function(int, float)",
    		" <i>Hello </i>\n" +
    		" <li>some list item \n" +
    		" </li>This is attention! \n" +
    		" You were warned! \n" +
    		" @author lots of them \n" +
    		" @author Zubr \n" +
    		" <b>boldword </b>\n" +
    		" Some brief description, \n" +
    		" extended to many lines. \n" +
    		" Not everything works right now... \n" +
    		" <code>codeword </code>\n" +
    		" <i>citationword </i>\n" +
    		" {@code some test code }\n" +
    		" Conditional comment: SOMECONDITION \n" +
    		" Some conditional comment \n" +
    		" End of conditional comment.\n" +
    		" Copyright: some copyright \n" +
    		" 1970 - 2012 \n" +
    		" @deprecated Now use another function \n" +
    		" This is very large \n" +
    		" and detailed description of some thing \n" +
    		" <i>italicword </i>\n" +
    		" <i>emphazedWord </i>\n" +
    		" @exception SuperError \n" +
    		" This will only appear in hmtl \n" +
    		" If: ANOTHERCONDITION {\n" +
    		" First part of comment \n" +
    		" If: SECONDCONDITION {\n" +
    		" Nested condition text \n" +
    		" }Else if: THIRDCONDITION {\n" +
    		" The third condition text \n" +
    		" }Else:  {The last text block \n" +
    		" }\n" +
    		" }Else:  {Second part of comment \n" +
    		" If: CONDITION {\n" +
    		" Second part extended \n" +
    		" }\n" +
    		" }\n" +
    		" If not: SOMECONDITION {\n" +
    		" This is printed if not \n" +
    		" }\n" +
    		" <img src=testImage.bmp alt=\"Hello, world!\" />\n" +
    		" Some text \n" +
    		" describing invariant. \n" +
    		" This will only appear in LATeX \n" +
    		" <ul> \n" +
    		" <li>Some unordered list \n" +
    		" </li><li>With lots of items \n" +
    		" </li><li>lots of lots of items \n" +
    		" </li></ul> \n" +
    		" {@link someMember Some description follows }\n" +
    		" This will only appear in man \n" +
    		" Note: Here \n" +
    		" is the note! \n" +
    		" This is an overloaded member function, provided for convenience. It differs from the above function only in what argument(s) it accepts.\n" +
    		" <code>someword </code>\n" +
    		" @package superPackage \n" +
    		" <p alt=\"The paragraph title \">\n" +
    		" The paragraph text. \n" +
    		" Maybe even multiline \n" +
    		" </p>\n" +
    		" @param a the first param \n" +
    		" Remarks: Some remark text \n" +
    		" Remarks: Another remarks section \n" +
    		" @return Whatever \n" +
    		" @return it \n" +
    		" @return may return \n" +
    		" This will only appear in RTF \n" +
    		" @see someOtherMethod \n" +
    		" @see function \n" +
    		" Same as \n" +
    		" brief description \n" +
    		" @since version 0.0.0.1 \n" +
    		" @throws superException \n" +
    		" @throws RuntimeError \n" +
    		" TODO: Some very important task \n" +
    		" @param b B is mentioned again... \n" +
    		" {@literal \n" +
    		"very long \n" +
    		"text with tags <sometag> \n" +
    		" }\n" +
    		" @version 0.0.0.2 \n" +
    		" Warning: This is senseless! \n" +
    		" This will only appear in XML \n" +
    		" Here goes test of symbols: \n" +
    		" $ @ \\ & ~ < > # % \" . :: \n" +
    		" And here goes simple text \n" +
    		"");
    // and ask the parser to check comments for us
    System.exit(parser.check(wantedComments));
  }
}