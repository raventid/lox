package loxi;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

import loxi.Scanner;
import loxi.ErrorReporter;
import loxi.AstPrinter;
import loxi.Parser;
import loxi.Interpreter;

public class Main {
   private static final Interpreter interpreter = new Interpreter();
   static boolean hadError = false;
   static boolean showLexerOutput = false;
   static boolean showParserOutput = false;
   static boolean hadRuntimeError = false;

   public static void main(String[] args) throws IOException {
      System.out.println("");
      if (args.length > 1) {
         System.out.println("Usage: jlox [script]");
         System.exit(64);
      } else if (args.length == 1) {
         runFile(args[0]);
      } else {
         runPrompt();
      }
   }

   private static void runFile(String path) throws IOException {
      byte[] bytes = Files.readAllBytes(Paths.get(path));
      run(new String(bytes, Charset.defaultCharset()));

      // Indicate an error in the exit code.
      if (hadError) { System.exit(65); }
      if (hadRuntimeError) System.exit(70);
   }

   private static void runPrompt() throws IOException {
      InputStreamReader input = new InputStreamReader(System.in);
      BufferedReader reader = new BufferedReader(input);
      for (;;) {
         System.out.print("> ");
         String line = reader.readLine();
         if (line == null) {
            break;
         }
         // If we receive command, than we should handle command
         if (commandCode(line) == 1) {
            if (showLexerOutput) {
               showLexerOutput = false;
               System.out.println("Lexer output has been disabled");
            } else {
               showLexerOutput = true;
               System.out.println("Lexer output has been enabled");
            }
         } else if(commandCode(line) == 2) {
            if (showParserOutput) {
               showParserOutput = false;
               System.out.println("Parser output has been disabled");
            } else {
               showParserOutput = true;
               System.out.println("Parser output has been enabled");
            }
         // Else we should handle loxi code
         } else {
            run(line);
         }
         hadError = false;
      }
   }

   private static void run(String source) {
      Scanner scanner = new Scanner(source, new ErrorReporter());
      List<Token> tokens = scanner.scanTokens();
      if (showLexerOutput) {
         for (Token token : tokens) {
            System.out.println(token);
         }

      }

      Parser parser = new Parser(tokens);
      Expr expression = parser.parse();

      if (hadError) { return; }

      if (showParserOutput) {
         System.out.println(new AstPrinter().print(expression));
      }

      interpreter.interpret(expression);
   }

   // No command: -1
   // Print lexer output: 1
   private static int commandCode(String line) {
      if (line.length() < 2) { return -1; }
      if (line.substring(0, 2).equals(":l")) { return 1; }
      if (line.substring(0, 2).equals(":p")) { return 2; }
      return -1;
   }

   static void runtimeError(RuntimeError error) {
      System.err.println(error.getMessage() + "\n[line " + error.token.line + "]");
      hadRuntimeError = true;
   }
}
