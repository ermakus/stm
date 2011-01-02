#!/usr/bin/python

import sys, re
from os import path
from bisect import *

class Thread:
    def __init__(self, num):
	self.tid = num
        self.stack = []
	self.crashed = False

    def dump(self):
	tag = ""
	if self.crashed:
	   tag = "CRASHED"
	print "Thread %s: %s" % ( self.tid, tag)
        for frame in self.stack:
            frame.dump()        
	print ""

class StackFrame:

    def __init__(self):
        pass

    def dump(self):
        print "%s %s (%s)" % (self.module, self.sym_addr(), self.sym_name())

    def sym_addr(self):
	return (int( self.addr, 0 ) - int ( self.base, 0 ))

    def sym_name(self):
	symb = self.symbols()
	if not symb:
	    return "No symbols"
	sym = sorted(symb.iterkeys())
	key = bisect_right(sym, self.sym_addr()) - 1
        saddr = sym[key]
	return symb[saddr]

    def symbols(self):
	
	fname = self.module + ".sym"
	if not path.exists(fname):
	    return None
	
	symbols = {}

	for line in file(fname):
	    s = re.search('([0-9a-fA-F]+) (.+)', line )
	    if s:
		symbols[ int( "0x" + s.group(1), 0 ) ] = s.group(2)

	return symbols

class CrashReportParser:

    def __init__(self,filename):
        first = True
	self.threads = []
	self.modules = {}
	tid = 0
	self.cid = -1
	for line in file( filename ):
		s = re.search('Device ID: +(\w+)', line)
		if s:
			if first:
			    first = False
	                    continue
			self.dump()
			self.threads = []
			self.modules = {}
			tid = 0
			self.cid = -1
			continue
		s = re.search('Crashed Thread: +(\d+)', line)
		if s:
			self.cid = int(s.group(1))
			continue
		s = re.search('Thread (\d+).(Crashed)?', line)
		if s:
			tid = int(s.group(1))
			self.threads.append( Thread( tid ) )
			continue
		s = re.search('(\d+)( +)([A-Za-z0-9\.]+)( +)(0x[0-9a-fA-F]+)(.*)', line )
		if s:
			frame = StackFrame()
			frame.number = s.group(1)
		        frame.module = s.group(3)
			frame.addr = s.group(5)
			self.threads[tid].stack.append( frame )
			continue
		s = re.search('(0x[0-9a-fA-F]+) \- (0x[0-9a-fA-F]+)( +)([A-Za-z0-9\.]+)', line )
		if s:
			addr = s.group(1)
			mod = s.group(4)
			self.modules[ mod ] = addr
			continue

    def update(self):
	for thread in self.threads:
		if( thread.tid == self.cid ):
		    thread.crashed = True

		for frame in thread.stack:
			frame.base = self.modules[ frame.module ]
	

    def dump(self):
        self.update()
        for thread in self.threads:
            if thread.crashed:
	        thread.dump()


parser = CrashReportParser( sys.argv[1] );
parser.dump()
