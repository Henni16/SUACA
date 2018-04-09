/*BEGIN_LEGAL

Copyright (c) 2016 Intel Corporation

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

END_LEGAL */
/// @file xed-disas-pecoff.cpp

//// ONLY COMPILES IF -mno-cygwin is thrown on to GCC compilations

#if defined(_WIN32)

extern "C" {
  #include "headers.h"
}
#include <cassert>
#include <cstdio>


#include <windows.h>
#include <winnt.h>

// xed headers -- THESE MUST BE AFTER THE WINDOWS HEADERS

extern "C" {
#include "pecoff.h"
}

using namespace std;

static void
windows_error(const char* syscall,
              const char* filename)
{
  printf("Mapped file:: %s",syscall);
  printf(" for file %s failed: ", filename);
  switch (GetLastError())
    {
    case 2:
      printf("File not found");
      break;
    case 3:
      printf("Path not found");
      break;
    case 5:
      printf("Access denied");
      break;
    case 15:
      printf("Invalid drive");
      break;
    default:
      printf("error code %u",
             XED_STATIC_CAST(xed_uint32_t,GetLastError()));
      break;
    }
}

typedef int (WINAPI *fptr_t)(void*);



class pecoff_reader_t
{
  /// NT handle for the open file.
  void* file_handle_;

  /// NT handle for the memory mapping.
  void* map_handle_;

  void* base_;
  xed_bool_t okay_;
  xed_bool_t sixty_four_bit_;

  const IMAGE_FILE_HEADER* ifh;
  const IMAGE_SECTION_HEADER* hdr;
  const IMAGE_SECTION_HEADER* orig_hdr;
  unsigned int nsections;
  xed_uint64_t image_base;
  xed_bool_t verbose;


public:
  xed_uint32_t section_index;

  pecoff_reader_t(int arg_verbose=1)
  {
    verbose = arg_verbose;
    init();
  }
  ~pecoff_reader_t()
  {
    close();
  }

  void* base() const { return base_; }
  xed_bool_t okay() const { return okay_; }
  xed_bool_t sixty_four_bit() const { return sixty_four_bit_; }

  void
  init()
  {
    file_handle_ = INVALID_HANDLE_VALUE;
    map_handle_ = INVALID_HANDLE_VALUE;
    okay_ = false;
    sixty_four_bit_ = false;

    hdr=0;
    orig_hdr=0;
    nsections=0;
    image_base=0;
    section_index=0;
  }

  void
  close()
  {
    if (base_)
      {
        UnmapViewOfFile(base_);
      }
    if (map_handle_ != INVALID_HANDLE_VALUE)
      {
        CloseHandle(map_handle_);
      }
    if (file_handle_ != INVALID_HANDLE_VALUE)
      {
        CloseHandle(file_handle_);
      }

    init();
  }


  xed_bool_t
  map_region(const char* input_file_name,
             void*& vregion,
             xed_uint32_t& len)
  {
    okay_ = false;

    file_handle_ = CreateFile(input_file_name,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_NO_BUFFERING + FILE_ATTRIBUTE_READONLY,
                              NULL);

    if (file_handle_ == INVALID_HANDLE_VALUE)  {
      windows_error("CreateFile", input_file_name);
    }

    map_handle_ = CreateFileMapping(file_handle_,
                                    NULL,
                                    PAGE_READONLY,
                                    0,
                                    0,
                                    NULL);

    if (map_handle_ == INVALID_HANDLE_VALUE)   {
      windows_error("CreateFileMapping", input_file_name);
    }

    base_ = MapViewOfFile(map_handle_,
                          FILE_MAP_READ, 0, 0, 0);
    if (base_ != NULL)   {
      okay_ = true;
      vregion = base_;
      len = 0; //FIXME
      return true;
    }
    CloseHandle(map_handle_);
    map_handle_ = INVALID_HANDLE_VALUE;

    CloseHandle(file_handle_);
    file_handle_ = INVALID_HANDLE_VALUE;
    return false;

  }

  xed_bool_t read_header() {
    if (! parse_nt_file_header(&nsections, &image_base, &hdr)) {
      printf("Could not read nt file header");
      return false;
    }

    orig_hdr=hdr;
    return true;
  }




  xed_bool_t
  module_section_info(
                      const char* secname,
                      xed_uint8_t*& section_start,
                      xed_uint32_t& section_size,
                      xed_uint64_t& virtual_addr)
  {
    unsigned int i,ii;
    char my_name[IMAGE_SIZEOF_SHORT_NAME];
    unsigned int match_len = 0;

    // Extract the name into a 0-padded 8 byte string.
    if (secname) {
      memset(my_name,0,IMAGE_SIZEOF_SHORT_NAME);
      for( i=0;i<IMAGE_SIZEOF_SHORT_NAME;i++)   {
        my_name[i] = secname[i];
        if (secname[i] == 0)
          break;
      }
      // match the substring that starts with the given target_section_name
      match_len = static_cast<unsigned int>(strlen(secname));
      if (match_len > IMAGE_SIZEOF_SHORT_NAME)
        match_len = IMAGE_SIZEOF_SHORT_NAME;
    }

    // There are section names that LOOK like .text$x but they really have
    // a null string embedded in them. So when you strcmp, you hit the
    // null.


    for ( ii = section_index; ii < nsections; ii++, hdr++)
    {
      int found = 0;
      if (hdr->SizeOfRawData == 0)
        continue;
      /* If no section name, match codde sections.  If we have a
         section name that matches , just disasssemble whatever they
         want. */
      if (secname==0) {
          if ((hdr->Characteristics & IMAGE_SCN_CNT_CODE) ==
              IMAGE_SCN_CNT_CODE)
              found = 1;
      }
      else if (strncmp(reinterpret_cast<const char*>(hdr->Name),
                       my_name, match_len) == 0)
      {
          found = 1;
      }
      if (found) {
            // Found it.  Extract the info and return.
            virtual_addr  = hdr->VirtualAddress  + image_base;
            section_size = (hdr->Misc.VirtualSize > 0 ?
                            hdr->Misc.VirtualSize
                            : hdr->SizeOfRawData);
            section_start = (xed_uint8_t*)ptr_add(base_,
                                                  hdr->PointerToRawData);
            section_index = ii+1;
            hdr++;
            return true;
      }
    }
    section_index = ii;
    return false;
  }

private:
  static inline const void*
  ptr_add(const void* ptr, unsigned int n)  {
    return static_cast<const char*>(ptr)+n;
  }


  xed_bool_t
  is_valid_module()  {
    // Point to the DOS header and check it.
    const IMAGE_DOS_HEADER* dh = static_cast<const IMAGE_DOS_HEADER*>(base_);
    if (dh->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    // Point to the PE signature word and check it.
    const DWORD* sig = static_cast<const DWORD*>(ptr_add(base_, dh->e_lfanew));

    // This must be a valid PE file with a valid DOS header.
    if (*sig != IMAGE_NT_SIGNATURE)
        return false;

    return true;
  }

  xed_bool_t
  parse_nt_file_header(unsigned int* pnsections,
                       xed_uint64_t* pimage_base,
                       const IMAGE_SECTION_HEADER** phdr)
  {
    // Oh joy - the format of a .obj file on Windows is *different*
    // from the format of a .exe file.  Deal with that.

    // Check the header to see if this is a valid .exe file
    if (is_valid_module())
      {
        // Point to the DOS header.
        const IMAGE_DOS_HEADER* dh =
            static_cast<const IMAGE_DOS_HEADER*>(base_);

        // Point to the COFF File Header (just after the signature)
        ifh = static_cast<const IMAGE_FILE_HEADER*>(ptr_add(base_,
                                                            dh->e_lfanew + 4));
      }
    else
      {
        // Maybe this is a .obj file, which starts with the image file header
        ifh = static_cast<const IMAGE_FILE_HEADER*>(base_);
      }



#if !defined(IMAGE_FILE_MACHINE_AMD64)
# define IMAGE_FILE_MACHINE_AMD64 0x8664
#endif

    if (ifh->Machine == IMAGE_FILE_MACHINE_I386) {
        sixty_four_bit_ = false;
    }
    else if (ifh->Machine == IMAGE_FILE_MACHINE_AMD64) {
        sixty_four_bit_ = true;
    }
    else  {
        // We only support Windows formats on IA32 and Intel64
        return false;
    }

    *pimage_base = 0;

    // Very important to use the 32b header here because the
    // unqualified IMAGE_OPTIONAL_HEADER gets the wrong version on
    // win64!
    const IMAGE_OPTIONAL_HEADER32* opthdr32
      = static_cast<const IMAGE_OPTIONAL_HEADER32*>(ptr_add(ifh, sizeof(*ifh)));

    // Cygwin's w32api winnt.h header doesn't distinguish 32 and 64b.
#if !defined(IMAGE_NT_OPTIONAL_HDR32_MAGIC)
# define IMAGE_NT_OPTIONAL_HDR32_MAGIC IMAGE_NT_OPTIONAL_HDR_MAGIC
#endif
    // And it lacks the definition for 64b headers
#if !defined(IMAGE_NT_OPTIONAL_HDR64_MAGIC)
# define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20b
#endif

    // Point to the first of the Section Headers
    *phdr = static_cast<const IMAGE_SECTION_HEADER*>(ptr_add(opthdr32,
                                                  ifh->SizeOfOptionalHeader));
    *pnsections = ifh->NumberOfSections;
    return true;
  }
};

////////////////////////////////////////////////////////////////////////////


void
process_pecoff(xed_uint8_t* start,
               unsigned int length,
               xed_disas_info_t decode_info,
               pecoff_reader_t reader,
               inst_list_t* instructions)
{

  xed_uint8_t* section_start = 0;
  xed_uint32_t section_size = 0;
  xed_uint64_t runtime_vaddr  = 0;

  xed_bool_t okay = true;
  xed_bool_t found = false;


  while(okay) {

      okay = reader.module_section_info(decode_info.target_section,
                                        section_start,
                                        section_size,
                                        runtime_vaddr);
      if (okay) {
          found = true;

          decode_info.s = XED_REINTERPRET_CAST(unsigned char*,start);
          decode_info.a =
              XED_REINTERPRET_CAST(unsigned char*,section_start);
          decode_info.q = XED_REINTERPRET_CAST(unsigned char*,
                                      section_start + section_size);

          decode_info.runtime_vaddr = runtime_vaddr + decode_info.fake_base;
          decode_info.runtime_vaddr_disas_start = decode_info.addr_start;
          decode_info.runtime_vaddr_disas_end = decode_info.addr_end;

          xed_disas_test(&decode_info, instructions);
      }
  }
  if (!found)
      printf("text section not found");

  (void) length;
}




extern "C" void
xed_disas_pecoff(xed_disas_info_t* fi, inst_list_t* instructions)
{
  xed_uint8_t* region = 0;
  void* vregion = 0;
  xed_uint32_t len = 0;
  pecoff_reader_t image_reader(fi->xml_format==0);
  xed_bool_t okay = image_reader.map_region(fi->input_file_name,
                                            vregion,
                                            len);
  if (!okay)
    printf("image read failed");
  image_reader.read_header();

  region = XED_REINTERPRET_CAST(xed_uint8_t*,vregion);

  if (image_reader.sixty_four_bit() &&
      fi->sixty_four_bit == 0 &&
      fi->use_binary_mode)
  {
      /* modify the default dstate values because we were not expecting a
       * 64b binary */
      fi->dstate.mmode = XED_MACHINE_MODE_LONG_64;
  }

  process_pecoff(region, len,  *fi, image_reader, instructions);
}

#endif