/**
 * Copyright (C) 2014 University of Virginia. All rights reserved.
 *
 * @file      packet-vcmtp.h
 * @author    Shawn Chen <sc7cq@virginia.edu>
 * @version   1.0
 * @date      Jan 28, 2015
 *
 * @section   LICENSE
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or（at your option）
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details at http://www.gnu.org/copyleft/gpl.html
 *
 * @brief     Dissector plugin of wireshark, used to parse VCMTP packet header.
 */


#include "config.h"
#include <epan/packet.h>

#define VCMTP_PORT 5173

/* packet types in the flag field */
#define VCMTP_BOP        0x00000001
#define VCMTP_EOP        0x00000002
#define VCMTP_MEM_DATA   0x00000004
#define VCMTP_RETX_REQ   0x00000008
#define VCMTP_RETX_REJ   0x00000010
#define VCMTP_RETX_END   0x00000020
#define VCMTP_RETX_DATA  0x00000040
#define VCMTP_BOP_REQ    0x00000080


/* register the packet data structure */
static int proto_vcmtp = -1;
static int hf_vcmtp_prodindex = -1;
static int hf_vcmtp_seqnum = -1;
static int hf_vcmtp_paylen = -1;
static int hf_vcmtp_flags = -1;
static int hf_vcmtp_flag_bop = -1;
static int hf_vcmtp_flag_eop = -1;
static int hf_vcmtp_flag_memdata = -1;
static int hf_vcmtp_flag_retxreq = -1;
static int hf_vcmtp_flag_retxrej = -1;
static int hf_vcmtp_flag_retxend = -1;
static int hf_vcmtp_flag_retxdata = -1;
static int hf_vcmtp_flag_bopreq = -1;
static gint ett_vcmtp = -1;


/**
 * The actual VCMTP packet dissector. Upon receiving a new incoming packet from
 * the network, wireshark will automatically split the known protocol header,
 * for example, MAC, IPv4 and UDP, then pass in a pointer to the remaining
 * payload in the buffer. Besides, it also passes in two other pointers, one
 * for containing metadata of the packet, another pointing at the parsing tree
 * of this protocol.
 */
static void dissect_vcmtp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    /* offset of the payload content */
    gint offset = 0;
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "VCMTP");
    /* Clear out stuff in the info column */
    col_clear(pinfo->cinfo,COL_INFO);
    if (tree) {
        proto_item *ti = NULL;
        proto_tree *vcmtp_tree = NULL;

        /* if the parsing tree exists, add fields to the tree */
        ti = proto_tree_add_item(tree, proto_vcmtp, tvb, 0, -1, ENC_NA);
        vcmtp_tree = proto_item_add_subtree(ti, ett_vcmtp);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_prodindex, tvb, offset, 4,
                            ENC_BIG_ENDIAN);
        offset += 4;
        /* increment the offset for parsing the next field */
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_seqnum, tvb, offset, 4,
                            ENC_BIG_ENDIAN);
        offset += 4;
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_paylen, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        offset += 2;
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flags, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_bop, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_eop, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_memdata, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_retxreq, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_retxrej, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_retxend, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_retxdata, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flag_bopreq, tvb, offset, 2,
                            ENC_BIG_ENDIAN);
        offset += 2;
    }
}


/**
 * The register function of this protocol. By specifying the detailed data
 * structure in each field, the dissector can have a clear view of the whole
 * content and thus parse the tree with this array. In this array, it requires
 * the field name, filter name, data type (length), base2/8/10/16, sub
 * structure and its value. The array inside this function is assocaited with
 * the dissector function.
 */
void proto_register_vcmtp(void)
{
    static hf_register_info hf[] = {
        /* Data structure of product index field */
        { &hf_vcmtp_prodindex,
            { "VCMTP ProdIndex", "vcmtp.prodindex",
            FT_UINT32, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        /* Data structure of sequence number field */
        { &hf_vcmtp_seqnum,
            { "VCMTP Sequence Number", "vcmtp.seqnum",
            FT_UINT32, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        /* Data structure of payload length field */
        { &hf_vcmtp_paylen,
            { "VCMTP Payload Length", "vcmtp.paylen",
            FT_UINT16, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        /* Data structure of flags field */
        { &hf_vcmtp_flags,
            { "VCMTP Flags", "vcmtp.flags",
            FT_UINT16, BASE_HEX,
            NULL, 0x0,
            NULL, HFILL }
        },
        /* Begin of product type, sub-structure of flags field */
        { &hf_vcmtp_flag_bop,
            { "VCMTP BOP Flag", "vcmtp.flags.bop",
            FT_BOOLEAN, 16,
            NULL, VCMTP_BOP,
            NULL, HFILL }
        },
        /* End of product type, sub-structure of flags field */
        { &hf_vcmtp_flag_eop,
            { "VCMTP EOP Flag", "vcmtp.flags.eop",
            FT_BOOLEAN, 16,
            NULL, VCMTP_EOP,
            NULL, HFILL }
        },
        /* Data block type, sub-structure of flags field */
        { &hf_vcmtp_flag_memdata,
            { "VCMTP MEM DATA Flag", "vcmtp.flags.memdata",
            FT_BOOLEAN, 16,
            NULL, VCMTP_MEM_DATA,
            NULL, HFILL }
        },
        /* Retx request type, sub-structure of flags field */
        { &hf_vcmtp_flag_retxreq,
            { "VCMTP RETX REQ Flag", "vcmtp.flags.retxreq",
            FT_BOOLEAN, 16,
            NULL, VCMTP_RETX_REQ,
            NULL, HFILL }
        },
        /* Retx request reject type, sub-structure of flags field */
        { &hf_vcmtp_flag_retxrej,
            { "VCMTP RETX REJ Flag", "vcmtp.flags.retxrej",
            FT_BOOLEAN, 16,
            NULL, VCMTP_RETX_REJ,
            NULL, HFILL }
        },
        /* Data block retx end type, sub-structure of flags field */
        { &hf_vcmtp_flag_retxend,
            { "VCMTP RETX END Flag", "vcmtp.flags.retxend",
            FT_BOOLEAN, 16,
            NULL, VCMTP_RETX_END,
            NULL, HFILL }
        },
        /* Data block retx type, sub-structure of flags field */
        { &hf_vcmtp_flag_retxdata,
            { "VCMTP RETX DATA Flag", "vcmtp.flags.retxdata",
            FT_BOOLEAN, 16,
            NULL, VCMTP_RETX_DATA,
            NULL, HFILL }
        },
        /* BOP request type, sub-structure of flags field */
        { &hf_vcmtp_flag_bopreq,
            { "VCMTP BOP REQ Flag", "vcmtp.flags.bopreq",
            FT_BOOLEAN, 16,
            NULL, VCMTP_BOP_REQ,
            NULL, HFILL }
        }
    };

    /* Setup protocol subtree array */
    static gint *ett[] = {
        &ett_vcmtp
    };

    proto_vcmtp = proto_register_protocol (
        "VCMTP Protocol", /* name       */
        "VCMTP",          /* short name */
        "vcmtp"           /* abbrev     */
    );

    proto_register_field_array(proto_vcmtp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}


/**
 * Handoff function. Underlying protocol and port number (from the standard
 * stack) should be specified here.
 */
void proto_reg_handoff_vcmtp(void)
{
    static dissector_handle_t vcmtp_handle;

    vcmtp_handle = create_dissector_handle(dissect_vcmtp, proto_vcmtp);
    dissector_add_uint("udp.port", VCMTP_PORT, vcmtp_handle);
}
