#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bfd.h"
#include "external.h"
#include "internal.h"


// Offsets within the Ehdr e_ident field.

const int EI_MAG0 = 0;
const int EI_MAG1 = 1;
const int EI_MAG2 = 2;
const int EI_MAG3 = 3;
const int EI_CLASS = 4;
const int EI_DATA = 5;
const int EI_VERSION = 6;
const int EI_OSABI = 7;
const int EI_ABIVERSION = 8;
const int EI_PAD = 9;
//const int EI_NIDENT = 16;

enum
{
  ELFCLASSNONE = 0,
  ELFCLASS32 = 1,
  ELFCLASS64 = 2
};

enum
{
  ELFDATANONE = 0,
  ELFDATA2LSB = 1,
  ELFDATA2MSB = 2
};


static int is_32bit_elf;

#if __STDC_VERSION__ >= 199901L || (defined(__GNUC__) && __GNUC__ >= 2)
/* We can't use any bfd types here since readelf may define BFD64 and
   objdump may not.  */
typedef unsigned long long dwarf_vma;
typedef unsigned long long dwarf_size_type;
#else
typedef unsigned long dwarf_vma;
typedef unsigned long dwarf_size_type;
#endif
dwarf_vma (*byte_get) (unsigned char *, int);

#define BYTE_GET(field)	byte_get (field, sizeof (field))

#define _(x) (x)

static void
__dump_data(unsigned char *ptr, int len, const char *func, int line)
{
    int i;
    printf(" =============dump=============\n");
    printf("\n [%s][%d]", func, line);
    for (i = 0; i < len; i++) {
        if (!(i%16))
            printf("\n %04x", i);
        printf(" %02x", ptr[i]);
    }
    printf("\n ==============================\n");
}

#define __DUMP_MEM__(p, len) \
    __dump_data(p, len, __func__, __LINE__)
void *
cmalloc (size_t nmemb, size_t size)
{
  /* Check for overflow.  */
  if (nmemb >= ~(size_t) 0 / size)
    return NULL;
  else
    return malloc (nmemb * size);
}

dwarf_vma
byte_get_big_endian (unsigned char *field, int size)
{
  switch (size)
    {
    case 1:
      return *field;

    case 2:
      return ((unsigned int) (field[1])) | (((int) (field[0])) << 8);

    case 4:
      return ((unsigned long) (field[3]))
	|   (((unsigned long) (field[2])) << 8)
	|   (((unsigned long) (field[1])) << 16)
	|   (((unsigned long) (field[0])) << 24);

    case 8:
      if (sizeof (dwarf_vma) == 8)
	return ((dwarf_vma) (field[7]))
	  |   (((dwarf_vma) (field[6])) << 8)
	  |   (((dwarf_vma) (field[5])) << 16)
	  |   (((dwarf_vma) (field[4])) << 24)
	  |   (((dwarf_vma) (field[3])) << 32)
	  |   (((dwarf_vma) (field[2])) << 40)
	  |   (((dwarf_vma) (field[1])) << 48)
	  |   (((dwarf_vma) (field[0])) << 56);
      else if (sizeof (dwarf_vma) == 4)
	{
	  /* Although we are extracing data from an 8 byte wide field,
	     we are returning only 4 bytes of data.  */
	  field += 4;
	  return ((unsigned long) (field[3]))
	    |   (((unsigned long) (field[2])) << 8)
	    |   (((unsigned long) (field[1])) << 16)
	    |   (((unsigned long) (field[0])) << 24);
	}

    default:
      printf ("Unhandled data length: %d\n", size);
      abort ();
    }
}

static long archive_file_offset;
static void *
get_data (void *var, FILE *file, long offset, size_t size, size_t nmemb,
	  const char *reason)
{
    void *mvar;

    if (size == 0 || nmemb == 0)
        return NULL;

    printf("%s %d ---------------\n", __func__, __LINE__);
    if (fseek (file, archive_file_offset + offset, SEEK_SET))
    {
        printf (_("Unable to seek to 0x%lx for %s\n"),
                (unsigned long) archive_file_offset + offset, reason);
        return NULL;
    }

    printf("%s %d ---------------\n", __func__, __LINE__);
    mvar = var;
    if (mvar == NULL)
    {
        /* Check for overflow.  */
        if (nmemb < (~(size_t) 0 - 1) / size)
            /* + 1 so that we can '\0' terminate invalid string table sections.  */
            mvar = malloc (size * nmemb + 1);

        if (mvar == NULL)
        {
            printf (_("Out of memory allocating 0x%lx bytes for %s\n"),
                    (unsigned long)(size * nmemb), reason);
            return NULL;
        }

        ((char *) mvar)[size * nmemb] = '\0';
    }

    printf("%s %d ---------------\n", __func__, __LINE__);
    if (fread (mvar, size, nmemb, file) != nmemb)
    {
        printf(_("Unable to read in 0x%lx bytes of %s\n"),
                (unsigned long)(size * nmemb), reason);
        if (mvar != var)
            free (mvar);
        return NULL;
    }
    printf("%s %d ---------------\n", __func__, __LINE__);

    return mvar;
}

static Elf_Internal_Ehdr elf_header;
static Elf_Internal_Shdr *section_headers;
static char *string_table;
static unsigned long string_table_length;

static int
get_32bit_section_headers (FILE *file, unsigned int num)
{
    Elf32_External_Shdr *shdrs;
    Elf_Internal_Shdr *internal;
    unsigned int i;

    shdrs = get_data (NULL, file, elf_header.e_shoff,
            elf_header.e_shentsize, num, _("section headers"));
    if (!shdrs)
        return 0;

    section_headers = cmalloc (num, sizeof (Elf_Internal_Shdr));

    if (section_headers == NULL)
    {
        printf(_("Out of memory\n"));
        return 0;
    }

    for (i = 0, internal = section_headers;
            i < num;
            i++, internal++)
    {
        internal->sh_name      = BYTE_GET (shdrs[i].sh_name);
        internal->sh_type      = BYTE_GET (shdrs[i].sh_type);
        internal->sh_flags     = BYTE_GET (shdrs[i].sh_flags);
        internal->sh_addr      = BYTE_GET (shdrs[i].sh_addr);
        internal->sh_offset    = BYTE_GET (shdrs[i].sh_offset);
        internal->sh_size      = BYTE_GET (shdrs[i].sh_size);
        internal->sh_link      = BYTE_GET (shdrs[i].sh_link);
        internal->sh_info      = BYTE_GET (shdrs[i].sh_info);
        internal->sh_addralign = BYTE_GET (shdrs[i].sh_addralign);
        internal->sh_entsize   = BYTE_GET (shdrs[i].sh_entsize);
        //__DUMP_MEM__(shdrs[i].sh_size, 4);
    }

    free (shdrs);

    return 1;
}

#define SECTION_NAME(X)	\
  ((X) == NULL ? "<none>" \
  : string_table == NULL ? "<no-name>" \
  : ((X)->sh_name >= string_table_length ? "<corrupt>" \
  : string_table + (X)->sh_name))

// The valid values found in the Shdr sh_type field.

enum SHT
{
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_HASH = 5,
  SHT_DYNAMIC = 6,
  SHT_NOTE = 7,
  SHT_NOBITS = 8,
  SHT_REL = 9,
  SHT_SHLIB = 10,
  SHT_DYNSYM = 11,
  SHT_INIT_ARRAY = 14,
  SHT_FINI_ARRAY = 15,
  SHT_PREINIT_ARRAY = 16,
  SHT_GROUP = 17,
  SHT_SYMTAB_SHNDX = 18,
  SHT_LOOS = 0x60000000,
  SHT_HIOS = 0x6fffffff,
  SHT_LOPROC = 0x70000000,
  SHT_HIPROC = 0x7fffffff,
  SHT_LOUSER = 0x80000000,
  SHT_HIUSER = 0xffffffff,
  // The remaining values are not in the standard.
  // Object attributes.
  SHT_GNU_ATTRIBUTES = 0x6ffffff5,
  // GNU style dynamic hash table.
  SHT_GNU_HASH = 0x6ffffff6,
  // List of prelink dependencies.
  SHT_GNU_LIBLIST = 0x6ffffff7,
  // Versions defined by file.
  SHT_SUNW_verdef = 0x6ffffffd,
  SHT_GNU_verdef = 0x6ffffffd,
  // Versions needed by file.
  SHT_SUNW_verneed = 0x6ffffffe,
  SHT_GNU_verneed = 0x6ffffffe,
  // Symbol versions,
  SHT_SUNW_versym = 0x6fffffff,
  SHT_GNU_versym = 0x6fffffff,

  SHT_SPARC_GOTDATA = 0x70000000,

  // Link editor is to sort the entries in this section based on the
  // address specified in the associated symbol table entry.
  SHT_ORDERED = 0x7fffffff,
};

/* How to print a vma value.  */
typedef enum print_mode
{
  HEX,
  DEC,
  DEC_5,
  UNSIGNED,
  PREFIX_HEX,
  FULL_HEX,
  LONG_HEX
}
print_mode;

/* Print a VMA value.  */
static int
print_vma (bfd_vma vma, print_mode mode)
{
  int nc = 0;

  switch (mode)
    {
    case FULL_HEX:
      nc = printf ("0x");
      /* Drop through.  */

    case LONG_HEX:
#ifdef BFD64
      if (is_32bit_elf)
	return nc + printf ("%8.8" BFD_VMA_FMT "x", vma);
#endif
      printf_vma (vma);
      return nc + 16;

    case DEC_5:
      if (vma <= 99999)
	return printf ("%5" BFD_VMA_FMT "d", vma);
      /* Drop through.  */

    case PREFIX_HEX:
      nc = printf ("0x");
      /* Drop through.  */

    case HEX:
      return nc + printf ("%" BFD_VMA_FMT "x", vma);

    case DEC:
      return printf ("%" BFD_VMA_FMT "d", vma);

    case UNSIGNED:
      return printf ("%" BFD_VMA_FMT "u", vma);
    }
  return 0;
}

#define STT_NOTYPE	0		/* Symbol type is unspecified */
#define STT_OBJECT	1		/* Symbol is a data object */
#define STT_FUNC	2		/* Symbol is a code object */
#define STT_SECTION	3		/* Symbol associated with a section */
#define STT_FILE	4		/* Symbol gives a file name */
#define STT_COMMON	5		/* An uninitialised common block */
#define STT_TLS		6		/* Thread local data object */
#define STT_RELC	8		/* Complex relocation expression */
#define STT_SRELC	9		/* Signed Complex relocation expression */
#define STT_LOOS	10		/* OS-specific semantics */
#define STT_HIOS	12		/* OS-specific semantics */
#define STT_LOPROC	13		/* Application-specific semantics */
#define STT_HIPROC	15		/* Application-specific semantics */

// Symbol binding from Sym st_info field.

enum STB
{
  STB_LOCAL = 0,
  STB_GLOBAL = 1,
  STB_WEAK = 2,
  STB_LOOS = 10,
  STB_HIOS = 12,
  STB_LOPROC = 13,
  STB_HIPROC = 15
};

enum EM
{
  EM_NONE = 0,
  EM_M32 = 1,
  EM_SPARC = 2,
  EM_386 = 3,
  EM_68K = 4,
  EM_88K = 5,
  // 6 used to be EM_486
  EM_860 = 7,
  EM_MIPS = 8,
  EM_S370 = 9,
  EM_MIPS_RS3_LE = 10,
  // 11 was the old Sparc V9 ABI.
  // 12 through 14 are reserved.
  EM_PARISC = 15,
  // 16 is reserved.
  // Some old PowerPC object files use 17.
  EM_VPP500 = 17,
  EM_SPARC32PLUS = 18,
  EM_960 = 19,
  EM_PPC = 20,
  EM_PPC64 = 21,
  EM_S390 = 22,
  // 23 through 35 are served.
  EM_V800 = 36,
  EM_FR20 = 37,
  EM_RH32 = 38,
  EM_RCE = 39,
  EM_ARM = 40,
  EM_ALPHA = 41,
  EM_SH = 42,
  EM_SPARCV9 = 43,
  EM_TRICORE = 44,
  EM_ARC = 45,
  EM_H8_300 = 46,
  EM_H8_300H = 47,
  EM_H8S = 48,
  EM_H8_500 = 49,
  EM_IA_64 = 50,
  EM_MIPS_X = 51,
  EM_COLDFIRE = 52,
  EM_68HC12 = 53,
  EM_MMA = 54,
  EM_PCP = 55,
  EM_NCPU = 56,
  EM_NDR1 = 57,
  EM_STARCORE = 58,
  EM_ME16 = 59,
  EM_ST100 = 60,
  EM_TINYJ = 61,
  EM_X86_64 = 62,
  EM_PDSP = 63,
  EM_PDP10 = 64,
  EM_PDP11 = 65,
  EM_FX66 = 66,
  EM_ST9PLUS = 67,
  EM_ST7 = 68,
  EM_68HC16 = 69,
  EM_68HC11 = 70,
  EM_68HC08 = 71,
  EM_68HC05 = 72,
  EM_SVX = 73,
  EM_ST19 = 74,
  EM_VAX = 75,
  EM_CRIS = 76,
  EM_JAVELIN = 77,
  EM_FIREPATH = 78,
  EM_ZSP = 79,
  EM_MMIX = 80,
  EM_HUANY = 81,
  EM_PRISM = 82,
  EM_AVR = 83,
  EM_FR30 = 84,
  EM_D10V = 85,
  EM_D30V = 86,
  EM_V850 = 87,
  EM_M32R = 88,
  EM_MN10300 = 89,
  EM_MN10200 = 90,
  EM_PJ = 91,
  EM_OPENRISC = 92,
  EM_ARC_A5 = 93,
  EM_XTENSA = 94,
  EM_VIDEOCORE = 95,
  EM_TMM_GPP = 96,
  EM_NS32K = 97,
  EM_TPC = 98,
  // Some old picoJava object files use 99 (EM_PJ is correct).
  EM_SNP1K = 99,
  EM_ST200 = 100,
  EM_IP2K = 101,
  EM_MAX = 102,
  EM_CR = 103,
  EM_F2MC16 = 104,
  EM_MSP430 = 105,
  EM_BLACKFIN = 106,
  EM_SE_C33 = 107,
  EM_SEP = 108,
  EM_ARCA = 109,
  EM_UNICORE = 110,
  EM_ALTERA_NIOS2 = 113,
  EM_CRX = 114,
  // The Morph MT.
  EM_MT = 0x2530,
  // DLX.
  EM_DLX = 0x5aa5,
  // FRV.
  EM_FRV = 0x5441,
  // Infineon Technologies 16-bit microcontroller with C166-V2 core.
  EM_X16X = 0x4688,
  // Xstorym16
  EM_XSTORMY16 = 0xad45,
  // Renesas M32C
  EM_M32C = 0xfeb0,
  // Vitesse IQ2000
  EM_IQ2000 = 0xfeba,
  // NIOS
  EM_NIOS32 = 0xfebb
  // Old AVR objects used 0x1057 (EM_AVR is correct).
  // Old MSP430 objects used 0x1059 (EM_MSP430 is correct).
  // Old FR30 objects used 0x3330 (EM_FR30 is correct).
  // Old OpenRISC objects used 0x3426 and 0x8472 (EM_OPENRISC is correct).
  // Old D10V objects used 0x7650 (EM_D10V is correct).
  // Old D30V objects used 0x7676 (EM_D30V is correct).
  // Old IP2X objects used 0x8217 (EM_IP2K is correct).
  // Old PowerPC objects used 0x9025 (EM_PPC is correct).
  // Old Alpha objects used 0x9026 (EM_ALPHA is correct).
  // Old M32R objects used 0x9041 (EM_M32R is correct).
  // Old V850 objects used 0x9080 (EM_V850 is correct).
  // Old S/390 objects used 0xa390 (EM_S390 is correct).
  // Old Xtensa objects used 0xabc7 (EM_XTENSA is correct).
  // Old MN10300 objects used 0xbeef (EM_MN10300 is correct).
  // Old MN10200 objects used 0xdead (EM_MN10200 is correct).
};

static const char *
get_symbol_type (unsigned int type)
{
    static char buff[32];

    switch (type)
    {
        case STT_NOTYPE:	return "NOTYPE";
        case STT_OBJECT:	return "OBJECT";
        case STT_FUNC:	return "FUNC";
        case STT_SECTION:	return "SECTION";
        case STT_FILE:	return "FILE";
        case STT_COMMON:	return "COMMON";
        case STT_TLS:	return "TLS";
        case STT_RELC:      return "RELC";
        case STT_SRELC:     return "SRELC";
        default:
                            if (type >= STT_LOPROC && type <= STT_HIPROC)
                            {
                                /*
                                   if (elf_header.e_machine == EM_ARM && type == STT_ARM_TFUNC)
                                   return "THUMB_FUNC";

                                if (elf_header.e_machine == EM_SPARCV9 && type == STT_REGISTER)
                                    return "REGISTER";

                                if (elf_header.e_machine == EM_PARISC && type == STT_PARISC_MILLI)
                                    return "PARISC_MILLI";

                                snprintf (buff, sizeof (buff), _("<processor specific>: %d"), type);
                            }
                            else if (type >= STT_LOOS && type <= STT_HIOS)
                            {
                                if (elf_header.e_machine == EM_PARISC)
                                {
                                    if (type == STT_HP_OPAQUE)
                                        return "HP_OPAQUE";
                                    if (type == STT_HP_STUB)
                                        return "HP_STUB";
                                }

                                snprintf (buff, sizeof (buff), _("<OS specific>: %d"), type);
                            }
                            else
                                snprintf (buff, sizeof (buff), _("<unknown>: %d"), type);
                                   */
                                snprintf (buff, sizeof (buff), _("<unknown>: %d"), type);
                            return buff;
    }
}

#define ELF_ST_BIND(val)		(((unsigned int)(val)) >> 4)
#define ELF_ST_TYPE(val)		((val) & 0xF)
#define ELF_ST_INFO(bind,type)		(((bind) << 4) + ((type) & 0xF))

#define ELF_ST_VISIBILITY(v)		((v) & 0x3)

static const char *
get_symbol_binding (unsigned int binding)
{
    static char buff[32];

    switch (binding)
    {
        case STB_LOCAL:	return "LOCAL";
        case STB_GLOBAL:	return "GLOBAL";
        case STB_WEAK:	return "WEAK";
        default:
                        if (binding >= STB_LOPROC && binding <= STB_HIPROC)
                            snprintf (buff, sizeof (buff), _("<processor specific>: %d"),
                                    binding);
                        else if (binding >= STB_LOOS && binding <= STB_HIOS)
                            snprintf (buff, sizeof (buff), _("<OS specific>: %d"), binding);
                        else
                            snprintf (buff, sizeof (buff), _("<unknown>: %d"), binding);
                        return buff;
    }
}

// Symbol visibility from Sym st_other field.

enum STV
{
  STV_DEFAULT = 0,
  STV_INTERNAL = 1,
  STV_HIDDEN = 2,
  STV_PROTECTED = 3
};

static const char *
get_symbol_visibility (unsigned int visibility)
{
    switch (visibility)
    {
        case STV_DEFAULT:	return "DEFAULT";
        case STV_INTERNAL:	return "INTERNAL";
        case STV_HIDDEN:	return "HIDDEN";
        case STV_PROTECTED: return "PROTECTED";
        default: abort ();
    }
}

#define STO_OPTIONAL		(1 << 2)
#define STO_MIPS16		0xf0
#define STO_MIPS_PLT		0x8
#define STO_MIPS_PIC		0x20

static const char *
get_mips_symbol_other (unsigned int other)
{
  switch (other)
    {
    case STO_OPTIONAL:  return "OPTIONAL";
    case STO_MIPS16:    return "MIPS16";
    case STO_MIPS_PLT:	return "MIPS PLT";
    case STO_MIPS_PIC:	return "MIPS PIC";
    default:      	return NULL;
    }
}

static const char *
get_symbol_other (unsigned int other)
{
  const char * result = NULL;
  static char buff [32];

  if (other == 0)
    return "";

  switch (elf_header.e_machine)
    {
    case EM_MIPS:
      result = get_mips_symbol_other (other);
    default:
      break;
    }

  if (result)
    return result;

  snprintf (buff, sizeof buff, _("<other>: %x"), other);
  return buff;
}

/* Small common symbol.  */
#define SHN_MIPS_SCOMMON	(SHN_LORESERVE + 3)

/* Small undefined symbol.  */
#define SHN_MIPS_SUNDEFINED	(SHN_LORESERVE + 4)

static const char *
get_symbol_index_type (unsigned int type)
{
  static char buff[32];

  switch (type)
    {
    case SHN_UNDEF:	return "UND";
    case SHN_ABS:	return "ABS";
    case SHN_COMMON:	return "COM";
    default:
                        /*
                        if (type == SHN_IA_64_ANSI_COMMON
                                && elf_header.e_machine == EM_IA_64
                                && elf_header.e_ident[EI_OSABI] == ELFOSABI_HPUX)
                            return "ANSI_COM";
                        else if (elf_header.e_machine == EM_X86_64
                                && type == SHN_X86_64_LCOMMON)
                            return "LARGE_COM";
                        else */if (type == SHN_MIPS_SCOMMON
                                && elf_header.e_machine == EM_MIPS)
                            return "SCOM";
                        else if (type == SHN_MIPS_SUNDEFINED
                                && elf_header.e_machine == EM_MIPS)
                            return "SUND";
                        else if (type >= SHN_LOPROC && type <= SHN_HIPROC)
                            sprintf (buff, "PRC[0x%04x]", type & 0xffff);
                        else if (type >= SHN_LOOS && type <= SHN_HIOS)
                            sprintf (buff, "OS [0x%04x]", type & 0xffff);
                        else if (type >= SHN_LORESERVE)
                            sprintf (buff, "RSV[0x%04x]", type & 0xffff);
                        else
                            sprintf (buff, "%3d", type);
                        break;
    }

  return buff;
}

int do_wide;
/* Display a symbol on stdout.  Handles the display of
   non-printing characters.
   If DO_WIDE is not true then format the symbol to be
   at most WIDTH characters, truncating as necessary.
   If WIDTH is negative then format the string to be
   exactly - WIDTH characters, truncating or padding
   as necessary.  */

#  define INT_MAX_32_BITS 2147483647
# ifndef INT_MAX
#  define INT_MAX INT_MAX_32_BITS
# endif



enum {
  /* In C99 */
  _sch_isblank  = 0x0001,	/* space \t */
  _sch_iscntrl  = 0x0002,	/* nonprinting characters */
  _sch_isdigit  = 0x0004,	/* 0-9 */
  _sch_islower  = 0x0008,	/* a-z */
  _sch_isprint  = 0x0010,	/* any printing character including ' ' */
  _sch_ispunct  = 0x0020,	/* all punctuation */
  _sch_isspace  = 0x0040,	/* space \t \n \r \f \v */
  _sch_isupper  = 0x0080,	/* A-Z */
  _sch_isxdigit = 0x0100,	/* 0-9A-Fa-f */

  /* Extra categories useful to cpplib.  */
  _sch_isidst	= 0x0200,	/* A-Za-z_ */
  _sch_isvsp    = 0x0400,	/* \n \r */
  _sch_isnvsp   = 0x0800,	/* space \t \f \v \0 */

  /* Combinations of the above.  */
  _sch_isalpha  = _sch_isupper|_sch_islower,	/* A-Za-z */
  _sch_isalnum  = _sch_isalpha|_sch_isdigit,	/* A-Za-z0-9 */
  _sch_isidnum  = _sch_isidst|_sch_isdigit,	/* A-Za-z0-9_ */
  _sch_isgraph  = _sch_isalnum|_sch_ispunct,	/* isprint and not space */
  _sch_iscppsp  = _sch_isvsp|_sch_isnvsp,	/* isspace + \0 */
  _sch_isbasic  = _sch_isprint|_sch_iscppsp     /* basic charset of ISO C
						   (plus ` and @)  */
};

/* Character classification.  */
extern const unsigned short _sch_istable[256];

#define _sch_test(c, bit) (_sch_istable[(c) & 0xff] & (unsigned short)(bit))

#define ISALPHA(c)  _sch_test(c, _sch_isalpha)
#define ISALNUM(c)  _sch_test(c, _sch_isalnum)
#define ISBLANK(c)  _sch_test(c, _sch_isblank)
#define ISCNTRL(c)  _sch_test(c, _sch_iscntrl)
#define ISDIGIT(c)  _sch_test(c, _sch_isdigit)
#define ISGRAPH(c)  _sch_test(c, _sch_isgraph)
#define ISLOWER(c)  _sch_test(c, _sch_islower)
#define ISPRINT(c)  _sch_test(c, _sch_isprint)
#define ISPUNCT(c)  _sch_test(c, _sch_ispunct)
#define ISSPACE(c)  _sch_test(c, _sch_isspace)
#define ISUPPER(c)  _sch_test(c, _sch_isupper)
#define ISXDIGIT(c) _sch_test(c, _sch_isxdigit)

#define ISIDNUM(c)	_sch_test(c, _sch_isidnum)
#define ISIDST(c)	_sch_test(c, _sch_isidst)
#define IS_ISOBASIC(c)	_sch_test(c, _sch_isbasic)
#define IS_VSPACE(c)	_sch_test(c, _sch_isvsp)
#define IS_NVSPACE(c)	_sch_test(c, _sch_isnvsp)
#define IS_SPACE_OR_NUL(c)	_sch_test(c, _sch_iscppsp)
static void
print_symbol (int width, const char *symbol)
{
  const char * format_string;
  const char * c;

  if (do_wide)
    {
      format_string = "%.*s";
      /* Set the width to a very large value.  This simplifies the code below.  */
      width = INT_MAX;
    }
  else if (width < 0)
    {
      format_string = "%-*.*2s";
      /* Keep the width positive.  This also helps.  */
      width = - width;
    }
  else
    {
      format_string = "%-.*s";
    }

  while (width)
    {
      int len;

      c = symbol;

      /* Look for non-printing symbols inside the symbol's name.
	 This test is triggered in particular by the names generated
	 by the assembler for local labels.  */
      while (ISPRINT (* c))
	c++;

      len = c - symbol;

      if (len)
	{
	  if (len > width)
	    len = width;
	  
	  printf (format_string, len, symbol);

	  width -= len;
	}

      if (* c == 0 || width == 0)
	break;

      /* Now display the non-printing character, if
	 there is room left in which to dipslay it.  */
      if (*c < 32)
	{
	  if (width < 2)
	    break;

	  printf ("^%c", *c + 0x40);

	  width -= 2;
	}
      else
	{
	  if (width < 6)
	    break;
	  
	  printf ("<0x%.2x>", *c);

	  width -= 6;
	}

      symbol = c + 1;
    }
}

static bfd_vma version_info[16];

int main (void)
{
    FILE *file = NULL;

    file = fopen("tr", "r");
    if (file == NULL) {
        return -1;
    }

    if (fread (elf_header.e_ident, EI_NIDENT, 1, file) != 1)
        return -1;

    switch (elf_header.e_ident[EI_DATA])
    {
        default: /* fall through */
        case ELFDATANONE: /* fall through */
        case ELFDATA2LSB:
            //byte_get = byte_get_little_endian;
            //byte_put = byte_put_little_endian;
            printf("%s %d LSB\n", __func__, __LINE__);
            break;
        case ELFDATA2MSB:
            byte_get = byte_get_big_endian;
            //byte_put = byte_put_big_endian;
            printf("%s %d MSB\n", __func__, __LINE__);
            break;
    }

    is_32bit_elf = (elf_header.e_ident[EI_CLASS] != ELFCLASS64);
    printf("%s %d is_32bit_elf : %d\n", __func__, __LINE__, is_32bit_elf);
    /* Read in the rest of the header.  */
    if (is_32bit_elf)
    {
        Elf32_External_Ehdr ehdr32;

        if (fread (ehdr32.e_type, sizeof (ehdr32) - EI_NIDENT, 1, file) != 1)
            return 0;

        elf_header.e_type      = BYTE_GET (ehdr32.e_type);
        elf_header.e_machine   = BYTE_GET (ehdr32.e_machine);
        elf_header.e_version   = BYTE_GET (ehdr32.e_version);
        elf_header.e_entry     = BYTE_GET (ehdr32.e_entry);
        elf_header.e_phoff     = BYTE_GET (ehdr32.e_phoff);
        elf_header.e_shoff     = BYTE_GET (ehdr32.e_shoff);
        elf_header.e_flags     = BYTE_GET (ehdr32.e_flags);
        elf_header.e_ehsize    = BYTE_GET (ehdr32.e_ehsize);
        elf_header.e_phentsize = BYTE_GET (ehdr32.e_phentsize);
        elf_header.e_phnum     = BYTE_GET (ehdr32.e_phnum);
        elf_header.e_shentsize = BYTE_GET (ehdr32.e_shentsize);
        elf_header.e_shnum     = BYTE_GET (ehdr32.e_shnum);
        elf_header.e_shstrndx  = BYTE_GET (ehdr32.e_shstrndx);
    }

    if (elf_header.e_shoff)
    {
        /* There may be some extensions in the first section header.  Don't
           bomb if we can't read it.  */
        if (is_32bit_elf)
            get_32bit_section_headers (file, elf_header.e_shnum);
        //else
            //get_64bit_section_headers (file, 1);
    }


    int i = 0;
    Elf_Internal_Shdr *section;

    /* Read in the string table, so that we have names to display.  */
    printf("%s %d ----section ndx:%d section num:%d----\n", __func__, __LINE__, elf_header.e_shstrndx, elf_header.e_shnum);
    if (elf_header.e_shstrndx != SHN_UNDEF
            && elf_header.e_shstrndx < elf_header.e_shnum)
    {
        section = section_headers + elf_header.e_shstrndx;
        printf("%s %d ---section:%x-----\n", __func__, __LINE__, (unsigned int)(unsigned long)section);

        printf("%s %d --section size:%d------\n", __func__, __LINE__, (unsigned int)section->sh_size);
        if (section->sh_size != 0)
        {
            printf("%s %d --string_table:%d------\n", __func__, __LINE__, (unsigned int)(unsigned long)string_table);
            string_table = get_data (NULL, file, section->sh_offset,
                    1, section->sh_size, _("string table"));
            printf("%s %d --string_table:%d------\n", __func__, __LINE__, (unsigned int)(unsigned long)string_table);

            string_table_length = string_table != NULL ? section->sh_size : 0;
        }
    }

    
    for (i = 0, section = section_headers;
            i < elf_header.e_shnum;
            i++, section++) {
        char *name = SECTION_NAME (section);
        printf("%s %d [%d] [%s] size:%d\n", __func__, __LINE__, i, name, (unsigned int)section->sh_size);
    }


    for (i = 0, section = section_headers;
            i < elf_header.e_shnum;
            i++, section++)
    {
        unsigned int si;
        char *strtab = NULL;
        unsigned long int strtab_size = 0;
        Elf_Internal_Sym *symtab;
        Elf_Internal_Sym *psym;


        if (   section->sh_type != SHT_SYMTAB
                && section->sh_type != SHT_DYNSYM)
            continue;

        printf (_("\nSymbol table '%s' contains %lu entries:\n"),
                SECTION_NAME (section),
                (unsigned long) (section->sh_size / section->sh_entsize));
        if (is_32bit_elf)
            printf (_("   Num:    Value  Size Type    Bind   Vis      Ndx Name\n"));
        else
            printf (_("   Num:    Value          Size Type    Bind   Vis      Ndx Name\n"));

        //symtab = GET_ELF_SYMBOLS (file, section);
        if (symtab == NULL)
            continue;

        if (section->sh_link == elf_header.e_shstrndx)
        {
            strtab = string_table;
            strtab_size = string_table_length;
        }
        else if (section->sh_link < elf_header.e_shnum)
        {
            Elf_Internal_Shdr *string_sec;

            string_sec = section_headers + section->sh_link;

            strtab = get_data (NULL, file, string_sec->sh_offset,
                    1, string_sec->sh_size, _("string table"));
            strtab_size = strtab != NULL ? string_sec->sh_size : 0;
        }

        for (si = 0, psym = symtab;
                si < section->sh_size / section->sh_entsize;
                si++, psym++)
        {
            printf ("%6d: ", si);
            print_vma (psym->st_value, LONG_HEX);
            putchar (' ');
            print_vma (psym->st_size, DEC_5);
            printf (" %-7s", get_symbol_type (ELF_ST_TYPE (psym->st_info)));
            printf (" %-6s", get_symbol_binding (ELF_ST_BIND (psym->st_info)));
            printf (" %-3s", get_symbol_visibility (ELF_ST_VISIBILITY (psym->st_other)));
            /* Check to see if any other bits in the st_other field are set.
               Note - displaying this information disrupts the layout of the
               table being generated, but for the moment this case is very rare.  */
            if (psym->st_other ^ ELF_ST_VISIBILITY (psym->st_other))
                printf (" [%s] ", get_symbol_other (psym->st_other ^ ELF_ST_VISIBILITY (psym->st_other)));
            printf (" %4s ", get_symbol_index_type (psym->st_shndx));
            print_symbol (25, psym->st_name < strtab_size
                    ? strtab + psym->st_name : "<corrupt>");

            if (section->sh_type == SHT_DYNSYM &&
                    //version_info[DT_VERSIONTAGIDX (DT_VERSYM)] != 0)
                    version_info[0] != 0)
            {
                unsigned char data[2];
                unsigned short vers_data;
                unsigned long offset;
                int is_nobits;
                int check_def;

                offset = offset_from_vma
                    //(file, version_info[DT_VERSIONTAGIDX (DT_VERSYM)],
                    (file, version_info[0],
                     sizeof data + si * sizeof (vers_data));

                get_data (&data, file, offset + si * sizeof (vers_data),
                        sizeof (data), 1, _("version data"));

                vers_data = byte_get (data, 2);

                is_nobits = (psym->st_shndx < elf_header.e_shnum
                        && section_headers[psym->st_shndx].sh_type
                        == SHT_NOBITS);

                check_def = (psym->st_shndx != SHN_UNDEF);

                if ((vers_data & 0x8000) || vers_data > 1)
                {
                    if (version_info[DT_VERSIONTAGIDX (DT_VERNEED)]
                            && (is_nobits || ! check_def))
                    {
                        Elf_External_Verneed evn;
                        Elf_Internal_Verneed ivn;
                        Elf_Internal_Vernaux ivna;

                        /* We must test both.  */
                        offset = offset_from_vma
                            (file, version_info[DT_VERSIONTAGIDX (DT_VERNEED)],
                             sizeof evn);

                        do
                        {
                            unsigned long vna_off;

                            get_data (&evn, file, offset, sizeof (evn), 1,
                                    _("version need"));

                            ivn.vn_aux  = BYTE_GET (evn.vn_aux);
                            ivn.vn_next = BYTE_GET (evn.vn_next);

                            vna_off = offset + ivn.vn_aux;

                            do
                            {
                                Elf_External_Vernaux evna;

                                get_data (&evna, file, vna_off,
                                        sizeof (evna), 1,
                                        _("version need aux (3)"));

                                ivna.vna_other = BYTE_GET (evna.vna_other);
                                ivna.vna_next  = BYTE_GET (evna.vna_next);
                                ivna.vna_name  = BYTE_GET (evna.vna_name);

                                vna_off += ivna.vna_next;
                            }
                            while (ivna.vna_other != vers_data
                                    && ivna.vna_next != 0);

                            if (ivna.vna_other == vers_data)
                                break;

                            offset += ivn.vn_next;
                        }
                        while (ivn.vn_next != 0);

                        if (ivna.vna_other == vers_data)
                        {
                            printf ("@%s (%d)",
                                    ivna.vna_name < strtab_size
                                    ? strtab + ivna.vna_name : "<corrupt>",
                                    ivna.vna_other);
                            check_def = 0;
                        }
                        else if (! is_nobits)
                            error (_("bad dynamic symbol\n"));
                        else
                            check_def = 1;
                    }

                    if (check_def)
                    {
                        if (vers_data != 0x8001
                                && version_info[DT_VERSIONTAGIDX (DT_VERDEF)])
                        {
                            Elf_Internal_Verdef ivd;
                            Elf_Internal_Verdaux ivda;
                            Elf_External_Verdaux evda;
                            unsigned long offset;

                            offset = offset_from_vma
                                (file,
                                 version_info[DT_VERSIONTAGIDX (DT_VERDEF)],
                                 sizeof (Elf_External_Verdef));

                            do
                            {
                                Elf_External_Verdef evd;

                                get_data (&evd, file, offset, sizeof (evd),
                                        1, _("version def"));

                                ivd.vd_ndx = BYTE_GET (evd.vd_ndx);
                                ivd.vd_aux = BYTE_GET (evd.vd_aux);
                                ivd.vd_next = BYTE_GET (evd.vd_next);

                                offset += ivd.vd_next;
                            }
                            while (ivd.vd_ndx != (vers_data & 0x7fff)
                                    && ivd.vd_next != 0);

                            offset -= ivd.vd_next;
                            offset += ivd.vd_aux;

                            get_data (&evda, file, offset, sizeof (evda),
                                    1, _("version def aux"));

                            ivda.vda_name = BYTE_GET (evda.vda_name);

                            if (psym->st_name != ivda.vda_name)
                                printf ((vers_data & 0x8000)
                                        ? "@%s" : "@@%s",
                                        ivda.vda_name < strtab_size
                                        ? strtab + ivda.vda_name : "<corrupt>");
                        }
                    }
                }
            }

            putchar ('\n');
        }

        free (symtab);
        if (strtab != string_table)
            free (strtab);
    }

    fclose(file);
    file = NULL;


    return 0;
}
