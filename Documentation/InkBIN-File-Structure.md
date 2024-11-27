# The InkCPP Binary Format: InkBIN

### WARNING: InkCPP is still in early development so this format is likely to change

As explained in the [[InkCPP compiler documentation|InkCPP-Compiler]], the InkCPP runtime does not load the same JSON created by the Inklecate compiler. Instead, it reads a custom binary compilation of that JSON data called InkBIN.

This document details the format of a compiled InkBIN file.

## A Note About Types

All integers, signed and unsigned, are stored in 4-byte. All floating point numbers are stored as signed 4-byte IEEE 754. All strings are encoded a sequence of null-terminated ASCII characters. 

# InkBIN

In order, the binary file contains...

## Byteorder Marker

```
uint16_t byteorder_marker;
```

A 2-byte integer that is always 0x0001. This is used to determine the endianness of the file.
If byteorder_marker is read as 0x0100, integer values must be byteswapped when being read. 

## Ink Version

```
int inkJsonVersion;
```

A 4-byte integer encoding the version of the Ink language this file was written in. This is copied over from the `inkVersion` field in the JSON input.

Soon this block will also include the version number of the InkCPP Compiler.

## Ink Binary Version

```
int inkBinVersion;
```

A 4-byte integer encoding the version of the InkCPP binary format. 

## The String List

```
First String\0
Second String\0
...
Final String\0\0
```

The string list is a series of null-terminated strings. Each corresponds to a unique string in the JSON input file. If a string is used multiple times in the JSON file, it is only stored once in the string table. Commands printing or loading strings will reference their location in this string list (see [[Commands|InkCPP-Commands]]).

The final string in the table ends with two null bytes instead of one, signalling the end of the string list.

## List definitions

```
list_flag firstFlagOfFirstList;
char[] first list name\0;
char[] flag name\0;
list_flag secondFlagOfFirstList;
char[] flag name\0;
...
list_flag lastFlagOfFirstList;
char[] flag name\0;
list_flag firstFlagOfSecondList;
char[] second list name\0;
char[] flag name\0;
...
list_flag lastFlagOfLastList;
char[] flag name\0;
null_flag
```

Defines all lists and flags.  A list flag entry contains a list id and the value of the flag.
The flags are sorted in increasing order grouped by the list id;

## The Lists list

```
flag flag flag null_flag
flag flag null_flag
...
flag flag nul_flag null_flag
```
The List list is a series of null_flag terminated entry lists.Each corresponds to a unique list in the JSON input.
If a list is referenced multiple times in the JSON file, it is multiple times stored in the list list, because the lists are
not named. Commands working with lists will refernce them by there offset in the list list (see [[InkCPP-Commands|InkCPP-Commands]]). When a list of length 0 is enounterd, the list parsing stops.

## Container Instruction Map

See the [[relevant section|Container-Metadata#Container Map]] of the [[Container Metadata|Container-Metadata]] document.

```
uint maxContainerIndex;
LIST OF SIZE (maxContainerIndex * 2)
    uint instructionOffset;
    uint containerIndex;
0x00000000
```

The first unsigned integer stores the maximum index of any container in the file. Since containers are sequentially numbered starting from 0, this is also the number of containers in the file.

Note that this number may not match the number of actual container structures in the InkJSON as only containers with turn or visit counting enabled are recorded.

Next, the map, which contains two entries for each container index. The first pair represents where the container begins; the second, where it ends. 

Again, check out the [[Container Metadata|Container-Metadata]] document to better understand this map and how it's used.

A four-byte unsigned 0 is marks the end of the container map.

## Container Hash Map

```
LIST OF
    uint containerNameHash;
    uint instructionOffset;
0x00000000
```

This section maps the hashes of container names to their starting offsets in the instruction data. This is only really needed for jumping to a path from code using the runner's `move_to` method.

Note that not all containers will appear in the map, as only some containers in Ink are "named containers." Anonymous containers have no names.

The map is a series of pairs. The first element of each pair is a 4-byte unsigned integer which is the hash of the containers fully qualified name (ex. "greatGrandParent.grandParent.parent.name"). The second element is an 4-byte unsigned integer representing the offset in the instruction data where this container starts (meaning that jumping to this instruction is equivalent to starting the container).

A four-byte unsigned 0 is signifies the end of the container hash map.

## Instructions

See the [[Commands page|InkCPP-Commands]] or [command.h](https://github.com/brwarner/inkcpp/blob/master/shared/private/command.h). for a full list of InkCPP commands.

```
LIST OF:
    ubyte CommandIndex;
    ubyte CommandFlag;
    (optional arguments depending on command)
```

A series of instructions. The first instruction represents the start of the ink file. 

The exact size of each instruction depends on the instruction itself. However, every instruction starts with the same two bytes.

The first byte is the command type corresponding to the Command enum in [command.h](https://github.com/brwarner/inkcpp/blob/master/shared/private/command.h).

The second byte is a command flag which may modify the behavior of the command. Most commands have no flags, so this is just 0. See ``CommandFlag`` in [command.h](https://github.com/brwarner/inkcpp/blob/master/shared/private/command.h) for the full list of flags. 

The command flag may contain multiple flags combined with a bitwise AND.

There is no EOF marker at the end of the command list. When you hit the end of the file, you have hit the final command.
