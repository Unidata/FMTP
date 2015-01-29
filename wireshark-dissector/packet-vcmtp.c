#include "config.h"

#include <epan/packet.h>

#define VCMTP_PORT 5173

static int proto_vcmtp = -1;
static int hf_vcmtp_prodindex = -1;
static int hf_vcmtp_seqnum = -1;
static int hf_vcmtp_paylen = -1;
static int hf_vcmtp_flags = -1;
static gint ett_vcmtp = -1;

static void dissect_vcmtp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "VCMTP");
    /* Clear out stuff in the info column */
    col_clear(pinfo->cinfo,COL_INFO);
    if (tree) { /* we are being asked for details */
        proto_item *ti = NULL;
        proto_tree *vcmtp_tree = NULL;

        ti = proto_tree_add_item(tree, proto_vcmtp, tvb, 0, -1, ENC_NA);
        vcmtp_tree = proto_item_add_subtree(ti, ett_vcmtp);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_prodindex, tvb, 0, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_seqnum, tvb, 0, 4, ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_paylen, tvb, 0, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(vcmtp_tree, hf_vcmtp_flags, tvb, 0, 2, ENC_BIG_ENDIAN);
    }
}

void proto_register_vcmtp(void)
{
    static hf_register_info hf[] = {
        { &hf_vcmtp_prodindex,
            { "VCMTP ProdIndex", "vcmtp.prodindex",
            FT_UINT32, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_vcmtp_seqnum,
            { "VCMTP Sequence Number", "vcmtp.seqnum",
            FT_UINT32, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_vcmtp_paylen,
            { "VCMTP Payload Length", "vcmtp.paylen",
            FT_UINT16, BASE_DEC,
            NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_vcmtp_flags,
            { "VCMTP Flags", "vcmtp.flags",
            FT_UINT16, BASE_DEC,
            NULL, 0x0,
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

void proto_reg_handoff_vcmtp(void)
{
    static dissector_handle_t vcmtp_handle;

    vcmtp_handle = create_dissector_handle(dissect_vcmtp, proto_vcmtp);
    dissector_add_uint("udp.port", VCMTP_PORT, vcmtp_handle);
}
