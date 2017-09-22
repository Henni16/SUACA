#include "util.h"


void xed_disas_info_init(xed_disas_info_t* p)
{
    memset(p,0,sizeof(xed_disas_info_t));
}


void init_xedd(xed_decoded_inst_t* xedd,
               xed_disas_info_t* di)
{
    xed_decoded_inst_zero_set_mode(xedd, &(di->dstate));
    xed_decoded_inst_set_input_chip(xedd, di->chip);
    if (di->operand != XED_OPERAND_INVALID)
        xed3_set_generic_operand(xedd, di->operand, di->operand_value);
}

static int
all_zeros(xed_uint8_t* p, unsigned int len)
{
    unsigned int i;
    for( i=0;i<len;i++)
        if (p[i])
            return 0;
    return 1;
}

static void XED_NORETURN
die_zero_len(
    xed_uint64_t runtime_instruction_address,
    unsigned char* z,
    xed_disas_info_t* di,
    xed_error_enum_t xed_error)
{
    printf("Zero length on decoded instruction!\n");
    /*xed_decode_error( runtime_instruction_address,
                      z-di->a, z, xed_error, 15);*/
    printf("Docode Error!\n");
    printf("[XED CLIENT ERROR] Dying\n");
    exit(1);
}

static char nibble_to_ascii_hex(xed_uint8_t i) {
    if (i<10) return i+'0';
    if (i<16) return i-10+'A';
    return '?';
}

void xed_print_hex_line(char* buf,
                        const xed_uint8_t* array,
                        const int length,
                        const int buflen)
{
  int n = length;
  int i=0;
  if (length == 0)
      n = XED_MAX_INSTRUCTION_BYTES;
  assert(buflen >= (2*n+1)); /* including null */
  for( i=0 ; i< n; i++)     {
      buf[2*i+0] = nibble_to_ascii_hex(array[i]>>4);
      buf[2*i+1] = nibble_to_ascii_hex(array[i]&0xF);
  }
  buf[2*i]=0;
}

static unsigned int
check_resync(xed_disas_info_t* di,
             xed_uint64_t runtime_instruction_address,
             unsigned int length,
             unsigned char* z)
{
    if (di->resync && di->symfn)
    {
        unsigned int x;
        for ( x=1 ; x<length ; x++ )
        {
            char* name = (*di->symfn)(runtime_instruction_address+x,
                                      di->caller_symbol_data);
            if (name)
            {
                printf("Error: Found Symbol in the middle of instruction\n");
                return x;
            }
        }
    }
    return 0;
}

void disassemble(xed_disas_info_t* di,
                 char* buf,
                 int buflen,
                 xed_decoded_inst_t* xedd,
                 xed_uint64_t runtime_instruction_address,
                 void* caller_data)
{
    int ok;
    xed_print_info_t pi;
    xed_init_print_info(&pi);
    pi.p = xedd;
    pi.blen = buflen;
    pi.buf = buf;

    // passed back to symbolic disassembly function
    pi.context = caller_data;

    // 0=use the default symbolic disassembly function registered via
    // xed_register_disassembly_callback(). If nonzero, it would be a
    // function pointer to a disassembly callback routine. See xed-disas.h
    pi.disassembly_callback = 0;

    pi.runtime_address = runtime_instruction_address;
    pi.syntax = XED_SYNTAX_INTEL;
    pi.format_options_valid = 1;
    pi.format_options = di->format_options;
    pi.buf[0]=0; //allow use of strcat

    ok = xed_format_generic(&pi);
    if (!ok)
    {
        pi.blen = xed_strncpy(pi.buf,"Error disassembling ",pi.blen);
        pi.blen = xed_strncat(pi.buf,
                               xed_syntax_enum_t2str(pi.syntax),
                               pi.blen);
        pi.blen = xed_strncat(pi.buf," syntax.",pi.blen);
    }
}


void xed_disas_test(xed_disas_info_t* di)
{
    // this decodes are region defined by the input structure.

    xed_uint64_t errors = 0;
    unsigned int m;
    unsigned char* z;
    unsigned char* zlimit;
    unsigned int length;
    int skipping;
    int last_all_zeros;
    unsigned int i;
    int okay;
    xed_decoded_inst_t xedd;
    xed_uint64_t runtime_instruction_address;
    xed_bool_t graph_empty = 1;
    unsigned int resync;


    m = di->ninst; // number of things to decode
    z = di->a;

    if (di->runtime_vaddr_disas_start)
        if (di->runtime_vaddr_disas_start > di->runtime_vaddr)
            z = (di->runtime_vaddr_disas_start - di->runtime_vaddr) +
                di->a;

    zlimit = 0;
    if (di->runtime_vaddr_disas_end) {
        if (di->runtime_vaddr_disas_end > di->runtime_vaddr)
            zlimit = (di->runtime_vaddr_disas_end - di->runtime_vaddr) +
                     di->a;
        else  /* end address is before start of this region -- skip it */
            return;
    }

    if (z >= di->q)   /* start pointer  is after end of section */
        return;

    // for skipping long strings of zeros
    skipping = 0;
    last_all_zeros = 0;
    int checkNext = 0;
    int analyse = 0;
    for( i=0; i<m;i++)
    {
        int ilim,elim;
        if (zlimit && z >= zlimit) {
            if (di->xml_format == 0)
                printf("# end of range.\n");
            break;
        }
        if (z >= di->q) {
            break;
        }

        /* if we get near the end of the section, clip the itext length */
        ilim = 15;
        elim = di->q - z;
        if (elim < ilim)
           ilim = elim;

        // if we get two full things of 0's in a row, start skipping.
        if (all_zeros((xed_uint8_t*) z, ilim))
        {
            if (skipping) {
                z = z + ilim;
                continue;
            }
            else if (last_all_zeros) {
                z = z + ilim;
                skipping = 1;
                continue;
            }
            else
                last_all_zeros = 1;
        }
        else
        {
            skipping = 0;
            last_all_zeros = 0;
        }

        runtime_instruction_address =  ((xed_uint64_t)(z-di->a)) +
                                       di->runtime_vaddr;

        okay = 0;
        length = 0;

        init_xedd(&xedd, di); // &(di->dstate), di->chip, di->mpx_mode);

            xed_uint64_t t1,t2;
            xed_error_enum_t xed_error = XED_ERROR_NONE;


            //do the decode
            xed_error = xed_decode(
                &xedd,
                XED_REINTERPRET_CAST(const xed_uint8_t*,z),
                ilim);

            xed_category_enum_t cat = xed_decoded_inst_get_category(&xedd);
            if (cat == XED_CATEGORY_DATAXFER){
              xed_int32_t imm = xed_decoded_inst_get_signed_immediate(&xedd);
              const xed_inst_t*           xi = xed_decoded_inst_inst(&xedd);
              const xed_operand_t*        op = xed_inst_operand(xi,0);
              xed_operand_enum_t     op_name = xed_operand_name(op);
              xed_reg_enum_t reg;
              xed3_get_generic_operand(&xedd,op_name,&reg);
              if ((imm == 111 || imm == 222) && reg == XED_REG_EBX){
                checkNext = 2;
              }
            }
            if (checkNext == 1 && cat == XED_CATEGORY_NOP) {
                unsigned int dec_len;
                char buffer[XED_HEX_BUFLEN];
                dec_len = xed_decoded_inst_get_length(&xedd);
                xed_print_hex_line(buffer, (xed_uint8_t*) z,
                                 dec_len, XED_HEX_BUFLEN);
                if (!strcmp(buffer, "646790"))
                  analyse = !analyse;
            }
            checkNext = checkNext == 2 ? 1 : 0;

            okay = (xed_error == XED_ERROR_NONE);

            length = xed_decoded_inst_get_length(&xedd);

            if (okay && length == 0)
                die_zero_len(runtime_instruction_address, z, di, xed_error);

            resync = check_resync(di, runtime_instruction_address, length, z);
            if (resync) {
                z += resync;
                continue;
            }

            if ((okay || xed_error == XED_ERROR_INVALID_FOR_CHIP)&&analyse)
            {
                // we still print it out if it is invalid for the chip.
                // so that people can see the problematic instruction
                /*emit_disasm(di, &xedd,
                            runtime_instruction_address,
                            z, gs, xed_error);*/
                char buffer[XED_TMP_BUF_LEN];
                disassemble(di, buffer, XED_TMP_BUF_LEN, &xedd,
                             runtime_instruction_address,
                             di->caller_symbol_data);
                printf("%s\n", buffer);
            }

            if (okay == 0 && analyse)
            {
                errors++;
                length = xed_decoded_inst_get_length(&xedd);
                if (length == 0)
                    length = 1;

                /*xed_decode_error( runtime_instruction_address,
                                  z-di->a,
                                  z,
                                  xed_error,
                                  length);*/
                printf("Decode Error!\n");

            }  // okay == 0
        z = z + length;
    }

}
