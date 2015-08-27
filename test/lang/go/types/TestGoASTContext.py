"""Test the go DWARF type parsing."""

import os, time
import unittest2
import lldb
import lldbutil
from lldbtest import *

class TestGoASTContext(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    #@skipUnlessGoInstalled
    @python_api_test
    def test_with_dsym_and_python_api(self):
        """Test GoASTContext dwarf parsing."""
        self.buildGo()
        self.objc_builtin_types()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line numbers to break inside main().
        self.main_source = "main.go"
        self.break_line = line_number(self.main_source, '// Set breakpoint here.')

    def objc_builtin_types(self):
        """Test expression parser respect for ObjC built-in types."""
        exe = os.path.join(os.getcwd(), "a.out")

        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target, VALID_TARGET)

        bpt = target.BreakpointCreateByLocation(self.main_source, self.break_line)
        self.assertTrue(bpt, VALID_BREAKPOINT)

        # Now launch the process, and do not stop at entry point.
        process = target.LaunchSimple (None, None, self.get_process_working_directory())

        self.assertTrue(process, PROCESS_IS_VALID)

        # The stop reason of the thread should be breakpoint.
        thread_list = lldbutil.get_threads_stopped_at_breakpoint (process, bpt)

        # Make sure we stopped at the first breakpoint.
        self.assertTrue (len(thread_list) != 0, "No thread stopped at our breakpoint.")
        self.assertTrue (len(thread_list) == 1, "More than one thread stopped at our breakpoint.")

        frame = thread_list[0].GetFrameAtIndex(0)
        self.assertTrue (frame, "Got a valid frame 0 frame.")

        var = frame.FindVariable('a')
        self.assertTrue(var, VALID_VARIABLE)
        self.assertEqual("int", var.type.name)

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
