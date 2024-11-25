#!/usr/bin/env python3

# A script to convert binary files to a C array
# Author: zwenergy

import sys
import math

if ( len( sys.argv ) != 3 ):
  print( "Wrong usage. Use of script:" )
  print( "bin2c.py INPUT OUTPUT" )
  exit( 1 )

srcFile = sys.argv[ 1 ]
dstFile = sys.argv[ 2 ]

outStr = []

with open( srcFile, "rb" ) as f:
  curByte = f.read( 1 )
  while ( curByte != b"" ):
    tmpInt = int.from_bytes( curByte, "big" )
    # Convert to hex string.
    outStr.append( format( tmpInt, '#04x' ) )
    
    # Next byte.
    curByte = f.read( 1 )

# Write array.
with open( dstFile, "w" ) as f:
  # First, define the address mask.
  addressBits = math.ceil( math.log2( len( outStr ) ) )
  
  curLine = "const unsigned char rom[ " + str( len( outStr ) ) + " ] = {\n"
  f.write( curLine )
  tmpCnt = 0
  firstLine = True
  for b in outStr:
    if ( not firstLine ):
      f.write( ", " )
      
    firstLine = False
    f.write( b )
    
    tmpCnt += 1
    
    if ( tmpCnt == 12 ):
      f.write( "\n" )
      tmpCnt = 0
  
  f.write( "\n};\n" )
