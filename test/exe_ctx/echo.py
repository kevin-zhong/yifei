#!/usr/bin/env python
#

import sys
import time, datetime
import math, random
import pprint

randCls = random.Random()
randCls.seed()

cnt = 0
while cnt < 12:
	width = randCls.randint(102, 10240)
	format = "{[--begin-%d %%%ds \n%d-end--]}\n"%(width, width, width)
	##print format
	print(format%("fffff "))
	time.sleep(0.2)
	cnt += 1
