import std.stdio;
import std.path;
import std.conv;
import std.getopt;
import std.algorithm;
import std.array;

import interpreter;
import loader;
import scheduler;
import vm;

int main(string[] args) {
    InterpreterMode interpreterMode = InterpreterMode.stack;
    uint timeSlice = 25;
    ushort checkAfter = 100;
    string loadPath = "./";

    string usage =
        "Usage: " ~ baseName(args[0]) ~ " [options] <module> <label> [<parameter> ...]\n" ~
        "Options:\n" ~
        "  -c <instructions>, --check-after=<instructions>\n" ~
        "    Check time slice timeout each number of <instructions> (" ~ to!string(checkAfter) ~ ")\n" ~
        "  -h, --help\n" ~
        "    Print this message and exit\n" ~
        "  -i, interpreter-mode <mode>\n" ~
        "    Start interpreter in 'stack' or 'register' <mode>\n" ~
        "  -l <directory>, --load-path=<directory>\n" ~
        "    Load POSM files from <directory> (" ~ loadPath ~ ")\n" ~
        "  -t <milli-seconds>, --time-slice=<milli-seconds>\n" ~
        "    <milli-seconds> spent by each job before context switch (" ~ to!string(timeSlice) ~ "ms)";

    enum ReturnCode : int {
        SUCCESS = 0,
        PARAMETER_ERROR = 1,
        LOADER_ERROR = 2,
        INTERPRETER_ERROR = 3,
        SCHEDULER_ERROR = 4,
        UNEXPECTED_ERROR = 5
    }

    try {
        auto helpInformation =
            args.getopt("t|time-slice", &timeSlice,
                        "c|check-after", &checkAfter,
                        "l|load-path", &loadPath,
                        "i|interpreter-mode", &interpreterMode);
        if (helpInformation.helpWanted) {
            stderr.writeln(usage);
            return ReturnCode.SUCCESS;
        }
    } catch (ConvException e) {
        stderr.writeln("Parameter error: " ~ e.msg);
        return ReturnCode.PARAMETER_ERROR;
    } catch (GetOptException e) {
        stderr.writeln("Parameter error: " ~ e.msg);
        stderr.writeln(usage);
        return ReturnCode.PARAMETER_ERROR;
    }

    string moduleName;
    uint label;
    long[] parameters;

    if (args.length < 3) {
        stderr.writeln(usage);
        return ReturnCode.PARAMETER_ERROR;
    }

    moduleName = args[1];
    try {
        label = to!uint(args[2]);
        parameters = args[3 .. $].map!(s => s.to!long).array;
    } catch (ConvException e) {
        stderr.writeln("Parameter error: " ~ e.msg);
        stderr.writeln(usage);
        return ReturnCode.PARAMETER_ERROR;
    }

    try {
        auto loader = new Loader(loadPath);
        auto interpreter = new Interpreter(loader, interpreterMode);
        auto scheduler =
            new Scheduler(loader, interpreter, timeSlice, checkAfter);
        interpreter.mspawn(loader, scheduler, moduleName, label, parameters);
        scheduler.run();
    } catch (LoaderError e) {
        stderr.writeln("Loader error: ", e.msg);
        return ReturnCode.LOADER_ERROR;
    } catch (InterpreterError e) {
        stderr.writeln("Interpreter error: ", e.msg);
        return ReturnCode.INTERPRETER_ERROR;
    } catch (SchedulerError e) {
        stderr.writeln("Scheduler error: ", e.msg);
        return ReturnCode.SCHEDULER_ERROR;
    } catch (VmError e) {
        stderr.writeln("Virtual machine error: ", e.msg);
        return ReturnCode.SCHEDULER_ERROR;
    } catch (Exception e) {
        stderr.writeln("Unexpected error: ", e.msg);
        return ReturnCode.UNEXPECTED_ERROR;
    }

    return ReturnCode.SUCCESS;
}
