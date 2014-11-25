"""
Test that the lldb-mi driver can interrupt and resume a looping app.
"""

import os
import unittest2
import lldb
from lldbtest import *

class MiInterruptTestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)
    myexe = "a.out"

    @classmethod
    def classCleanup(cls):
        """Cleanup the test byproducts."""
        try:
            os.remove("child_send.txt")
            os.remove("child_read.txt")
            os.remove(cls.myexe)
        except:
            pass

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @lldbmi_test
    def test_lldbmi_interrupt(self):
        """Test that 'lldb-mi --interpreter' interrupt and resume a looping app."""
        import pexpect
        self.buildDefault()

        # The default lldb-mi prompt (seriously?!).
        prompt = "(gdb)"

        # So that the child gets torn down after the test.
        self.child = pexpect.spawn('%s --interpreter' % (self.lldbMiExec))
        child = self.child
        child.setecho(True)
        # Turn on logging for input/output to/from the child.
        with open('child_send.txt', 'w') as f_send:
            with open('child_read.txt', 'w') as f_read:
                child.logfile_send = f_send
                child.logfile_read = f_read

                child.send("-file-exec-and-symbols " + self.myexe)
                child.sendline('')
                child.expect("\^done")

                #run to main
                child.send("-break-insert -f main")
                child.sendline('')
                child.expect("\^done,bkpt={number=\"1\"")
                child.send("-exec-run")
                child.sendline('') #FIXME: hangs here; extra return below is needed
                child.send("")
                child.sendline('')
                child.expect("\^running")
                child.expect("\*stopped,reason=\"breakpoint-hit\"")

                #set doloop=1 and run (to loop forever)
                child.send("-data-evaluate-expression \"doloop=1\"")
                child.sendline('')
                child.expect("value=\"1\"")
                child.send("-exec-continue")
                child.sendline('')
                child.expect("\^running")

                #issue interrupt, set a bp, and resume
                child.send("-exec-interrupt")
                child.sendline('')
                child.expect("\*stopped,reason=\"signal-received\"")
                child.send("-break-insert loop.c:11")
                child.sendline('')
                child.expect("\^done,bkpt={number=\"2\"")
                #child.send("-exec-resume")
                #child.sendline('') #FIXME: command not recognized
                child.send("-exec-continue")
                child.sendline('')
                child.expect("\*stopped,reason=\"breakpoint-hit\"")

                #we should be sitting at loop.c:12
                #set loop=-1 so we'll exit the loop
                child.send("-data-evaluate-expression \"loop=-1\"")
                child.sendline('')
                child.expect("value=\"-1\"")
                child.send("-exec-continue")
                child.sendline('')
                child.expect("\^running")
                child.expect("\*stopped,reason=\"exited-normally\"")
                child.expect_exact(prompt)

                child.send("quit")
                child.sendline('')

        # Now that the necessary logging is done, restore logfile to None to
        # stop further logging.
        child.logfile_send = None
        child.logfile_read = None
        
        with open('child_send.txt', 'r') as fs:
            if self.TraceOn():
                print "\n\nContents of child_send.txt:"
                print fs.read()
        with open('child_read.txt', 'r') as fr:
            from_child = fr.read()
            if self.TraceOn():
                print "\n\nContents of child_read.txt:"
                print from_child


if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
