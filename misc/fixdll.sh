#!/bin/sh
#
#  Shell script to scan the header files and build the DLL export definition files for the Windows port.
#
#  It uses misc/scanexp.c as a support for the preprocessing stage and can run in two modes:
#    - without option: generates the export definition files for MSVC, MinGW and BCC32
#    - with '--update-symbol-list': generates the reference symbol list misc/dllsyms.lst
#
#  If the reference symbol list misc/dllsyms.lst already exists, the script runs in an incremental way:
#  new symbols (those not listed in misc/dllsyms.lst) are appended at the end of the new symbol list.
#  This mechanism maintains DLL binary compatibility as long as symbols are only added and not removed.


# helper function for removing the temporary files
rm_temp_files() {
   rm -f _ord*.tmp
   rm -f _*syms*.tmp
   rm -f _*def*.tmp
   rm -f _all.def
}


# check that we are in the Allegro dir
if [ ! -f include/allegro5/allegro5.h ]; then
   echo "*** Error !!! ***"
   echo " you are not in the Allegro directory"
   exit 1
fi


# i18n changes the behaviour of sort (under Linux), which we don't want
export LC_ALL=C


# test for the presence of GNU sort
echo "_a" > _ord1.tmp
echo "a" >> _ord1.tmp
sort _ord1.tmp > _ord2.tmp
if { ! cmp -s _ord1.tmp _ord2.tmp; } then
   echo "*** Error !!! ***"
   echo " the first sort utility in the PATH is not GNU sort"
   rm_temp_files
   exit 1
fi


# scan the header files
echo "Scanning for API symbols..."
gcc -E -I. -I./include -DSCAN_EXPORT -DALLEGRO_API -o _apidef.tmp misc/scanexp.c
sed -n -e "s/^ *allexp[fi][un][nl]  *\**\(.*\)_sym.*/    \1/p" _apidef.tmp > _apidef1.tmp
sed -n -e "s/^ *allexp[vfa][apr][rtr]  *\**\(.*\)_sym.*/    \1 DATA/p" _apidef.tmp >> _apidef1.tmp
sort _apidef1.tmp > _apidef2.tmp

echo "Scanning for WinAPI symbols..."
gcc -E -I. -I./include -DSCAN_EXPORT -DALLEGRO_WINAPI -o _wapidef.tmp misc/scanexp.c
sed -n -e "s/^ *allexp[fi][un][nl]  *\**\(.*\)_sym.*/    \1/p" _wapidef.tmp > _wapidef1.tmp
sed -n -e "s/^ *allexp[vfa][apr][rtr]  *\**\(.*\)_sym.*/    \1 DATA/p" _wapidef.tmp >> _wapidef1.tmp
sort _wapidef1.tmp > _wapidef2.tmp

echo "Scanning for internal symbols..."
gcc -E -I. -I./include -DSCAN_EXPORT -DALLEGRO_INTERNALS -o _intdef.tmp misc/scanexp.c
sed -n -e "s/^ *allexp[fi][un][nl]  *\**\(.*\)_sym.*/    \1/p" _intdef.tmp > _intdef1.tmp
sed -n -e "s/^ *allexp[vfa][apr][rtr]  *\**\(.*\)_sym.*/    \1 DATA/p" _intdef.tmp >> _intdef1.tmp
sort _intdef1.tmp > _intdef2.tmp


# put together the newly generated symbol list
cat _apidef2.tmp _wapidef2.tmp _intdef2.tmp > _alldef2.tmp


# check against the reference symbol list if it exists
if [ -e misc/dllsyms.lst ]; then
   echo "Checking against existing symbol list..."
   # comm works only on sorted files
   sort _alldef2.tmp > _allsyms2.tmp
   sort misc/dllsyms.lst > _dllsyms2.tmp

   # test for missing symbols
   comm -1 -3 _allsyms2.tmp _dllsyms2.tmp > _difsyms2.tmp
   if [ -s _difsyms2.tmp ]; then
      echo " *** Error !!! ***"
      echo "  symbols missing => DLL binary compatibility broken"
      echo "  you need to add the following symbols:"
      cat _difsyms2.tmp
      rm_temp_files
      exit 1
   fi

   # test for new symbols
   comm -2 -3 _allsyms2.tmp _dllsyms2.tmp > _difsyms2.tmp
   if [ -s _difsyms2.tmp ]; then
      echo " "`wc --lines < _difsyms2.tmp`" symbol(s) added:"
      cat _difsyms2.tmp
      cat misc/dllsyms.lst _difsyms2.tmp > _alldef3.tmp
   else
      echo " no symbol added"
      cp misc/dllsyms.lst _alldef3.tmp
   fi
else
   cp _alldef2.tmp _alldef3.tmp
fi


# generate new reference symbol list or the DLL export definition files
if [ "$1" = "--update-symbol-list" ]; then
   echo "Updating symbol list..."
   cp -f _alldef3.tmp misc/dllsyms.lst
else
   # LIBRARY is necessary for DMC.
   echo "; generated by fixdll.sh" > _all.def
   echo "LIBRARY" >> _all.def
   echo "EXPORTS" >> _all.def
   sed -e "p" -e "=" -e "d" _alldef3.tmp > _alldef4.tmp
   sed -e "N" -e "s/\n/ @/" -e "s/DATA \(.*\)/\1 DATA/" _alldef4.tmp >> _all.def

   echo "Generating..."
   echo " lib/msvc/allegro.def"
   mkdir -p lib/msvc
   cp _all.def lib/msvc/allegro.def

   echo " lib/mingw32/allegro.def"
   mkdir -p lib/mingw32
   cp _all.def lib/mingw32/allegro.def

   echo " lib/dmc/allegro.def"
   mkdir -p lib/dmc
   cp _all.def lib/dmc/allegro.def

   echo " lib/bcc32/allegro.def"
   mkdir -p lib/bcc32
   sed -e "s/^    \([a-zA-Z0-9_]*\) \([@0-9]*\)\([ A-Z]*\)/    _\1 = \1/" _all.def > lib/bcc32/allegro.def

fi

rm_temp_files

echo "Done!"