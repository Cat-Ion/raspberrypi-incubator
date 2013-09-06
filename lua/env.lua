sandboxenv = {
   ipairs = ipairs,
   next = next,
   pairs = pairs,
   select = select,
   bit32 = bit32,
   math = math,
   os = {
	  clock = os.clock,
	  date = os.date,
	  difftime = os.difftime,
	  time = os.time 
   },
   string = string,
   table = table
}