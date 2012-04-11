Scott's Open Asset Import Library Fork
========


    Table of contents

	1.		Overview
	 1.1		Simple Model Format
	 1.2		Simple Scene Format
     1.3        Simple Binary Model Format
	3. 		Where to get help
	4.		License




### 1. Overview ###
This is a fork of the Open Asset Import Library. It includes a new exporter
that can be easily used in simple 3d formats. I created this due to my
frustration with common model formats - they are either excessively complicated
or too narrowly designed.

#### 1.1 Simple Model Format ####
The simple mesh format is exactly that - a very simple mesh format that is a
snap to read. Rather than a binary format, this format is a self documenting
XML format. I know some people will immediately see red with this statement,
but it makes the models trivally easy to load with tinyxml :)

#### 1.2 Simple Scene Format ####
The simple scene format is an extension of the simple model format. It adds
encodes additional information that composes a graphics scene.

#### 1.3 Simple Binary Model Format ###
No implementation yet. Very similiar to the Simple model format, only it is
in binary. Again, a lot of extra optimizations have been skipped in an attempt
to keep the format as simple as possible.

### 3. Where to get help ###
This is simply a fork of the asset importer project. Please see their website
for more information:

(http://assimp.sourceforge.net/)

### 4. License ###
The license of the Asset Import Library is based on the modified, __3-clause BSD__-License, which is a very liberal license. An _informal_ summary is: do whatever you want, but include Assimp's license text with your product - and don't sue us if our code doesn't work.
