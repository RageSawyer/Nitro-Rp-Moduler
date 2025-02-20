// y_uvars

#include <YSI\internal\y_version>

#include <YSI\y_amx>
#include <YSI\y_debug>
#include <YSI\y_utils>

#include <YSI\y_users>
#include <YSI\y_ini>

#include <YSI\y_hooks>

#include <YSI\internal\y_stripnumbers>

// Third "uvar" version.
#define _YU@LE@E%0>
#define _YU@LT@E%0> ;

// Needs two levels of indirection to strip the excess commas (,%0,%1).
#define _YU@LO(,%0,%1,%2) %0@yA_();public %0@yA_(){N@(#....#%0,_:%1,_:%2,STRIP_NUMBERS:%0[0]|||%2:0|||);}

#define _YU@LE%0[%1][%3]%2> _YU@LO(%0,%1,%3) _YU@LE%2>

// Recursive local default string definition.
#define _YU@LJ(,%0,%1,%2) %0[%1][%2]
#define _YU@LT%0[%1][%3]%2> ,_YU@LJ(%0,%1,%3)_YU@LT%2>
#define _YU@LA%0[%1][%3]%2> _YU@LJ(%0,%1,%3)_YU@LT%2>

// Entry point for the loaders.  The structure of stored pvar data is:
//  
//  [0] - Pointer to next pvar in list (-1 for end).
//  [1] - Pointer to data.
//  [2] - Number of players.
//  [3] - Size of enum.
//  [4] - Start of name.
//  
// It is VERY important to note that using "%0[0][0]" when calling "N@" instead
// of "%0" gives a DIFFERENT address - we get the address of the first data
// element in the array, not the address of the start of the array pointer table
// which is used to index multi-dimensional arrays when the size is not known
// (which in this case it is).  This makes calculating offsets later possible.
#define uvar%0[%8][%1]%2; stock _YU@LA,%0[%8][%1]%2@E>_YU@LE,%0[%8][%1]%2@E>

//%0@yA_();public%0@yA_()N@(_:%0,#....#%0 _YA@LT %1,@E|||);

// This is a structure defining the data stored on the enum structure.
enum E_USERS_FAKE_DATA
{
	E_USERS_FAKE_DATA_NEXT,
	E_USERS_FAKE_DATA_DATA,
	E_USERS_FAKE_DATA_LEN,
	E_USERS_FAKE_DATA_STR[2]
}

static stock
	YSI_g_sFirstUVarData = -1,
	// These three variables are used to speed up data loading through caching.
	YSI_g_sLastName[32] = "\1\0",
	YSI_g_sLastAddr,
	YSI_g_sLastPlayers,
	YSI_g_sLastSize;

forward _y_uvar_include_@();

public _y_uvar_include_@()
{
	memset("", 0, 0, 0);
	Player_WriteArray("", "", 0);
}

static stock Uvar_FindData(const name[], data[])
{
	// This function gets passed an empty string so that we can use "data" as a
	// string, while secretly changing the pointer in AMX code.
	new
		p = YSI_g_sFirstUVarData;
	while (p != -1)
	{
		// Modify our data pointer.
		#emit LOAD.S.pri p
		#emit STOR.S.pri data
		if (!strcmp(data[4], name))
		{
			strcpy(YSI_g_sLastName, name);
			YSI_g_sLastSize = data[3];
			YSI_g_sLastPlayers = data[2];
			YSI_g_sLastAddr = data[1];
			//printf("found %s, %d, %d, %d", YSI_g_sLastName, YSI_g_sLastSize, YSI_g_sLastPlayers, YSI_g_sLastAddr);
			return;
		}
		p = data[0];
	}
	YSI_g_sLastAddr = -1;
}

forward OnUserData[y_uvar](playerid, name[], value[]);
public OnUserData[y_uvar](playerid, name[], value[])
{
	// See what the name of the loaded data was.
	new
		pos = strfind(name, "-");
	if (pos == -1)
	{
		if (strcmp(name, YSI_g_sLastName))
		{
			// Find the data.
			Uvar_FindData(name, "");
		}
		if (YSI_g_sLastAddr == -1)
		{
			return;
		}
		// Check that the data is the right size.
		P:C(if (strval(value) != YSI_g_sLastSize) P:E("uvar data changed in %s", YSI_g_sLastName););
	}
	else
	{
		// Get the position in the array of this data.
		//printf("call pos 0");
		name[pos] = '\0';
		pos = strval(name[pos + 1]) * ((MAX_INI_ENTRY_TEXT - 1) / 16 * 3);
		if (strcmp(name[2], YSI_g_sLastName, false))
		{
			// Find the data.
			Uvar_FindData(name[2], "");
		}
		if (YSI_g_sLastAddr == -1)
		{
			return;
		}
		// Get the offset in the array for this player.
		if (playerid < YSI_g_sLastPlayers)
		{
			new
				len = strlen(value),
				idx;
			pos += YSI_g_sLastSize * playerid;
			// Save this pointer to an array variable for simplicity.
			#emit LOAD.pri   YSI_g_sLastAddr
			#emit STOR.S.pri name
			// "pos" holds the offset of this data.  "value" always holds a
			// whole number of cells worth of data.
			while (idx + 16 <= len)
			{
				// Do the large chunks.
				name[pos++] = ((value[idx +  0] - '>') << 26)
				            | ((value[idx +  1] - '>') << 20)
				            | ((value[idx +  2] - '>') << 14)
				            | ((value[idx +  3] - '>') <<  8)
				            | ((value[idx +  4] - '>') <<  2)
				            | ((value[idx +  5] - '>') >>  4);
				// Second cell.
				name[pos++] = ((value[idx +  5] - '>') << 28)
				            | ((value[idx +  6] - '>') << 22)
				            | ((value[idx +  7] - '>') << 16)
				            | ((value[idx +  8] - '>') << 10)
				            | ((value[idx +  9] - '>') <<  4)
				            | ((value[idx + 10] - '>') >>  2);
				// Third cell.
				name[pos++] = ((value[idx + 10] - '>') << 30)
				            | ((value[idx + 11] - '>') << 24)
				            | ((value[idx + 12] - '>') << 18)
				            | ((value[idx + 13] - '>') << 12)
				            | ((value[idx + 14] - '>') <<  6)
				            | ((value[idx + 15] - '>') >>  0);
				// 16 characters are used to encode 3 cells (12 bytes) by only
				// saving 6 bits per character to ensure that they are always
				// valid characters.  7 bits may be easier, but would mean the
				// encoding fit less well to small numbers of cells.
				idx += 16;
			}
			if (idx + 6 <= len)
			{
				// Save any few extra bytes.
				name[pos++] = ((value[idx +  0] - '>') << 26)
				            | ((value[idx +  1] - '>') << 20)
				            | ((value[idx +  2] - '>') << 14)
				            | ((value[idx +  3] - '>') <<  8)
				            | ((value[idx +  4] - '>') <<  2)
				            | ((value[idx +  5] - '>') >>  4);
				if (idx + 11 <= len)
				{
					name[pos++] = ((value[idx +  5] - '>') << 28)
					            | ((value[idx +  6] - '>') << 22)
					            | ((value[idx +  7] - '>') << 16)
					            | ((value[idx +  8] - '>') << 10)
					            | ((value[idx +  9] - '>') <<  4)
					            | ((value[idx + 10] - '>') >>  2);
				}
			}
		}
	}
}

/*----------------------------------------------------------------------------*\
Function:
	N@
Params:
	val[][] - Handle to the PAWN data array.
	volatile vardata[] - Handle to the memory location in which to store info.
	{K@, L@, M@, N@, _}:... - Array slot size information.
Return:
	-
Notes:
	This function modifies "vardata" well beyond its original limits to contain
	information on the structure of the enum used to define "val".  This code
	uses the name and size information passed in the additional parameters as
	strings, and makes assumptions about how the compiler lays out memory to
	combine all the passed strings in to one big string in what could be ROM,
	but in SA:MP isn't.  This takes a human readable(ish) description of the
	array elements and converts it in to a much simpler to read format for the
	computer to use later when loading and storing data.
	
	The description above is no longer the case.  This code now just saves the
	size of the data, the number of players in the array, the address of the
	data, a pointer to another data set and the name of this data.  This is by
	far much simpler than the old version.
\*----------------------------------------------------------------------------*/

stock N@(volatile const vardata[], playerCount, dataSize, &pointer)
{
	new
		sAddr;
	// Store the basic data, including linked-list pointers and a pointer to the
	// location at which the data is stored.
	#emit LOAD.S.pri vardata
	#emit STOR.S.pri sAddr
	printf("", YSI_g_sFirstUVarData);
	#emit LOAD.pri   YSI_g_sFirstUVarData
	#emit SREF.S.pri sAddr
	YSI_g_sFirstUVarData = sAddr;
	sAddr += 4;
	#emit LOAD.S.pri pointer
	#emit SREF.S.pri sAddr
	sAddr += 4;
	#emit LOAD.S.pri playerCount
	#emit SREF.S.pri sAddr
	sAddr += 4;
	#emit LOAD.S.pri dataSize
	#emit SREF.S.pri sAddr
	P:5("N@: %d %d %d %d %s", vardata[0], vardata[1], vardata[2], vardata[3], vardata[4]);
	P:5("N@: %d", YSI_g_sFirstUVarData);
}

hook OnScriptInit()
{
	// List them all.
	YSI_g_sFirstUVarData = -1;
	// Call all @yA_ functions to get all required data.
	new
		idx,
		buffer;
	while ((idx = AMX_GetPublicPointerSuffix(idx, buffer, _A<@yA_>)))
	{
		#emit PUSH.C     0
		#emit LCTRL      6
		#emit ADD.C      28
		#emit PUSH.pri
		#emit LOAD.S.pri buffer
		#emit SCTRL      6
	}
}

hook OnPlayerLogout(playerid, yid)
{
	// Loop through all the player data items and write them to a file.
	//static const
	//	sc_cellsPerWrite = 
	Player_SetTag("y_uvar");
	new
		p = YSI_g_sFirstUVarData,
		temp;
	while (p != -1)
	{
		// DO NOT CHANGE THE CODE BELOW HERE!!!
		// Call a function sort of.  This allows us to push an arbitrary address
		// as an array to a function.
		#emit LOAD.S.pri p
		// Get the max players.
		#emit ADD.C      8
		#emit STOR.S.pri temp
		#emit LREF.S.pri temp
		#emit STOR.S.pri temp
		if (playerid < temp)
		{
			// Get the data size.
			#emit LOAD.S.pri p
			#emit ADD.C      12
			#emit STOR.S.pri temp
			#emit LREF.S.pri temp
			#emit PUSH.pri
			// Get the data offset.
			#emit LOAD.S.alt playerid
			#emit SMUL
			#emit SMUL.C     4
			#emit MOVE.alt
			// Get the data pointer.
			#emit LOAD.S.pri p
			#emit ADD.C      4
			#emit STOR.S.pri temp
			#emit LREF.S.pri temp
			#emit ADD
			#emit PUSH.pri
			// Get the function name.
			#emit LOAD.S.pri p
			#emit ADD.C      16
			#emit PUSH.pri
			// Save the next pointer.
			#emit LREF.S.pri p
			#emit STOR.S.pri p
			// Now push the size of data put on the stack.
			#emit PUSH.C     12
			// Now get the return address and push it.
			#emit LCTRL      6
			#emit ADD.C      28
			#emit PUSH.pri
			// Call "Player_WriteArray" directly.
			#emit CONST.pri  Player_WriteArray
			#emit SCTRL      6
			// DO NOT CHANGE THE CODE ABOVE HERE!!!
		}
	}
}

hook OnPlayerConnect(playerid)
{
	P:1("hook Users_OnPlayerConnect called: %i", playerid);
	new
		p = YSI_g_sFirstUVarData,
		temp;
	while (p != -1)
	{
		// DO NOT CHANGE THE CODE BELOW HERE!!!
		// Call a function sort of.  This allows us to push an arbitrary address
		// as an array to a function.
		#emit LOAD.S.pri p
		// Get the max players.
		#emit ADD.C      8
		#emit STOR.S.pri temp
		#emit LREF.S.pri temp
		#emit STOR.S.pri temp
		if (playerid < temp)
		{
			// Get the data enum size.
			#emit PUSH.C     8
			#emit PUSH.C     0
			#emit LOAD.S.pri p
			#emit ADD.C      12
			#emit STOR.S.pri temp
			#emit LREF.S.pri temp
			#emit PUSH.pri
			// Get the data offset.
			#emit LOAD.S.alt playerid
			#emit SMUL
			#emit SMUL.C     4
			#emit MOVE.alt
			// Get the data pointer.
			#emit LOAD.S.pri p
			#emit ADD.C      4
			#emit STOR.S.pri temp
			#emit LREF.S.pri temp
			#emit ADD
			#emit PUSH.pri
			// Save the next pointer.
			#emit LREF.S.pri p
			#emit STOR.S.pri p
			// Now push the size of data put on the stack.
			#emit PUSH.C     16
			// Now get the return address and push it.
			#emit LCTRL      6
			#emit ADD.C      28
			#emit PUSH.pri
			// Call "memset" directly.
			#emit CONST.pri  memset
			#emit SCTRL      6
			// DO NOT CHANGE THE CODE ABOVE HERE!!!
		}
	}
}

#endinput

/*stock Users_Debug()
{
	// Print the first value from every array.
}*/

//#endinput

Users_DoDataPrint(const playerid, const uid, data[], len, const structure[])
{
	#pragma unused len
	// DO NOT CHANGE THE CODE BELOW HERE!!!
	// Find and save the information on this function for later.  We don't even
	// need any checks here because we are saving the address of the instruction
	// after this code then returning in an invisible way.
	#emit LCTRL    6
	#emit ADD.C    20
	#emit STOR.pri YSI_g_sDoDataPrintAddr
	#emit RETN
	// DO NOT CHANGE THE CODE ABOVE HERE!!!
	//new
	//	str[64];
	//strunpack(str, structure);
	//printf("%d, %d", playerid, uid);
	//printf("structure: %s, len: %d, data: %d %d", str, len, data[0], data[1]);
	P:5("Users_DoDataPrint: len: %d, data: %d %d", len, data[0], data[1]);
	P:C(for(new _i = 0, _j = strlen(structure); _i != _j; ++_i) {P:5("%d: %08x (%c", _i, structure[_i], structure[_i]);});
	//printf("%d, %d, %d, %s", data[E_USERS_FAKE_DATA_NEXT], data[E_USERS_FAKE_DATA_DATA], data[E_USERS_FAKE_DATA_LEN], str);
	//printf("hi");
	//return data[E_USERS_FAKE_DATA_NEXT];
	//return -1;
	// OK, let's get started on trying to write code to output these various
	// arrays, even if at this point they are only printed to the console, it is
	// still a step in the right direction (and with the speed I'm currently
	// going ANY step is a good step)!
	// Print the user data.
	if (IsPlayerConnected(playerid))
	{
		printf("Users_DoDataPrint: %d (%d) = %s", playerid, uid, ReturnPlayerName(playerid));
	}
	// Print the variable name.
	new
		namelen = structure[0],
		datalen,
		datatype,
		dataOffset = 0,
		indexOffset = 1;
	printf("Users_DoDataPrint: %d: %.*s", namelen, namelen, structure[indexOffset]);
	for ( ; ; )
	{
		indexOffset = indexOffset + namelen + 1;
		namelen = structure[indexOffset - 1];
		if (!namelen)
		{
			// If the returned length is 0, that means we have reached a null
			// character, and thus the end of the string.
			break;
		}
		else switch (namelen & 0xFF000000)
		{
			case 0x00000000:
			{
				printf("Variable: %d: %.*s", namelen, namelen, structure[indexOffset]);
				++dataOffset;
			}
			case 0x10000000:
			{
				datalen = structure[indexOffset++] + dataOffset;
				//datalen = structure[indexOffset++];
				datatype = structure[indexOffset++] - '0';
				// Remove the flag and excess length at the same time.
				namelen -= 0x10000002;
				printf("Special array %d: %d: %.*s (%d)", datatype, namelen, namelen, structure[indexOffset], datalen - dataOffset);
				switch (datatype)
				{
					case BitArray@:
					{
						printf("data = %s", Bit_Display(BitArray:data[dataOffset], bits<datalen>));
					}
				}
				dataOffset = datalen;
			}
			case 0x20000000:
			{
				datalen = structure[indexOffset++] + dataOffset;
				// Remove the flag and excess length at the same time.
				namelen -= 0x20000001;
				printf("Array: %d: %.*s (%d)", namelen, namelen, structure[indexOffset], datalen - dataOffset);
				for (new i = dataOffset; i != datalen; ++i)
				{
					printf("data[%d] = %d", i, data[i]);
				}
				dataOffset = datalen;
			}
			default:
			{
				// Error!
				P:E("Attempted to save unknown data type, failing!");
				return;
			}
		}
	}
	//printf("
}

hook OnPlayerLogout(playerid, uid)
{
//	Users_DoLogout(playerid, uid);
//}
//
//stock Users_DoLogout(playerid, uid)
//{
	P:1("hook Users_OnPlayerLogout called: %i, %i", playerid, uid);
	new
		p = YSI_g_sFirstUVarData,
		temp;
	while (p != -1)
	{
		// DO NOT CHANGE THE CODE BELOW HERE!!!
		// Call a function sort of.  This allows us to push an arbitrary address
		// as an array to a function.
		#emit LOAD.S.pri p
		// Get the structure.
		#emit ADD.C      12
		#emit PUSH.pri
		// Get the length.
		#emit ADD.C      0xFFFFFFFC // -4
		#emit STOR.S.pri temp
		#emit LREF.S.pri temp
		#emit PUSH.pri
		// Get the data offset.
		#emit LOAD.S.alt playerid
		#emit SMUL
		#emit SMUL.C     4
		#emit MOVE.alt
		// Get the data pointer.
		#emit LOAD.S.pri p
		#emit ADD.C      4
		#emit STOR.S.pri temp
		#emit LREF.S.alt temp
		#emit LOAD.S.pri playerid
		#emit IDXADDR
		#emit MOVE.alt
		#emit LOAD.i
		#emit ADD
		#emit PUSH.pri
		// Save the next pointer.
		#emit LREF.S.pri p
		#emit STOR.S.pri p
		// Push the other parameters.
		#emit PUSH.S     uid
		#emit PUSH.S     playerid
		// Now push the size of data put on the stack.
		#emit PUSH.C     20
		// Now get the return address and push it.
		#emit LCTRL      6
		#emit ADD.C      32
		#emit PUSH.pri
		// Now start the function to store certain data.
		#emit PROC
		// Now jump in to the middle of the function.
		#emit LOAD.pri   YSI_g_sDoDataPrintAddr
		#emit SCTRL      6
		// Now store the return value.
		/*#emit STOR.S.pri p*/
		// DO NOT CHANGE THE CODE ABOVE HERE!!!
	}
}

Users_DoDataReset(data[], len)
{
	// OK, since it says not to change anything here, I should briefly explain
	// what it does so that people know why not to change it.  This basically
	// gets the address of the code after the "#emit" blocks and stores that
	// address in a variable, then ends the function in a compiler-invisible
	// way.  This variable is used to call this function directly later on in
	// the code from more "#emit" blocks so that pure addresses can be passed
	// instead of having the compiler complain that a variable is not an array
	// (it isn't, but it holds a reference to an array, and the method used to
	// pass the variable means the run-time thinks this is correct).
	// DO NOT CHANGE THE CODE BELOW HERE!!!
	#emit LCTRL    6
	#emit ADD.C    20
	#emit STOR.pri YSI_g_sDoDataResetAddr
	#emit RETN
	// DO NOT CHANGE THE CODE ABOVE HERE!!!
	// Need "memset" really!  I have written a memset function based on looping
	// through a sub-set of an array, then using memcpy to copy that subset over
	// the rest of the array (which has shown nice speed-ups), but it needs more
	// testing to confirm that it does what I think it does.  I also need to
	// determine the optimal block size (for which I have a script written, I
	// just need to run it).
	/*while (len--)
	{
		data[len] = 0;
	}*/
	// The "0" is the default parameter, but I've specified it anyway.
	memset(data, len, 0);
}
