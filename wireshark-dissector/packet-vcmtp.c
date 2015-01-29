#include "config.h"

#include <epan/packet.h>

#define VCMTP_PORT 5173

static int proto_vcmtp = -1;

static void dissect_vcmtp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "VCMTP");
    /* Clear out stuff in the info column */
    col_clear(pinfo->cinfo,COL_INFO);
    if (tree) { /* we are being asked for details */
        proto_item *ti = NULL;
        ti = proto_tree_add_item(tree, proto_vcmtp, tvb, 0, -1, ENC_NA);
    }
}

void proto_register_vcmtp(void)
{
    proto_vcmtp = proto_register_protocol (
        "VCMTP Protocol", /* name       */
        "VCMTP",          /* short name */
        "vcmtp"           /* abbrev     */
    );
}

void proto_reg_handoff_vcmtp(void)
{
    static dissector_handle_t vcmtp_handle;

    vcmtp_handle = create_dissector_handle(dissect_vcmtp, proto_vcmtp);
    dissector_add_uint("udp.port", VCMTP_PORT, vcmtp_handle);
}
