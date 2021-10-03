package loxi;

class ErrorReporter {
    static void error(int line, String message) {
        report(line, "", message);
    }

    private static void report(int line, String where, String message) {
        System.out.println("[line " + line + "] Error" + where + ": " + message);
    }
}
