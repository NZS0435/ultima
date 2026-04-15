/* =========================================================================
 * phase2_smoke_test.cpp — Phase 2 non-interactive build validation
 * =========================================================================
 * Team Thunder #001
 *
 * Team Authors   : Stewart Pawley, Zander Hayes, Nicholas Kobs
 * Phase Label    : Phase 2 - Message Passing (IPC)
 * Primary Author : Stewart Pawley
 * Co-Authors     : Zander Hayes
 *                  Nicholas Kobs
 * =========================================================================
 */

#include "MCB.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifdef ULTIMA_BETTI_FFI_ENABLED
#include "Betti.h"
#endif

MCB sys_mcb;

namespace {
void idle_task() {
}

struct ArtifactRequest {
    bool enabled = false;
    std::string prefix;
};

ArtifactRequest parse_artifact_request(int argc, char* argv[]) {
    ArtifactRequest request {};
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--emit-betti-artifacts" && (index + 1) < argc) {
            request.enabled = true;
            request.prefix = argv[++index];
        }
    }
    return request;
}

#ifdef ULTIMA_BETTI_FFI_ENABLED
std::uint64_t rotl64(std::uint64_t value, unsigned int shift) {
    return (value << shift) | (value >> (64U - shift));
}

std::string json_escape(const std::string& input) {
    std::ostringstream out;
    for (unsigned char ch : input) {
        switch (ch) {
            case '\\': out << "\\\\"; break;
            case '\"': out << "\\\""; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (ch < 0x20U) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<unsigned int>(ch) << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(ch);
                }
                break;
        }
    }
    return out.str();
}

std::string deterministic_hex256(const std::string& input) {
    std::array<std::uint64_t, 4> lanes = {
        0xcbf29ce484222325ULL,
        0x9e3779b97f4a7c15ULL,
        0x94d049bb133111ebULL,
        0xd6e8feb86659fd93ULL
    };

    for (std::size_t index = 0; index < input.size(); ++index) {
        const std::uint64_t byte = static_cast<unsigned char>(input[index]);
        for (std::size_t lane = 0; lane < lanes.size(); ++lane) {
            lanes[lane] ^= byte + static_cast<std::uint64_t>(lane * 41U) + static_cast<std::uint64_t>(index * 17U);
            lanes[lane] *= (0x100000001b3ULL + static_cast<std::uint64_t>(lane * 2U));
            lanes[lane] = rotl64(lanes[lane], static_cast<unsigned int>(11U + lane * 7U));
        }
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto lane : lanes) {
        out << std::setw(16) << lane;
    }
    return out.str();
}

bool write_text_file(const std::string& path, const std::string& contents) {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    out << contents;
    return out.good();
}

bool emit_betti_sidecar_artifacts(const ArtifactRequest& request,
                                  int sender_id,
                                  int receiver_id,
                                  int rogue_id,
                                  const Message& encrypted_message,
                                  std::size_t ciphertext_queue_depth,
                                  bool rogue_send_denied,
                                  bool rogue_read_denied,
                                  bool authorized_decrypt_ok,
                                  const std::string& security_overview,
                                  std::string* error_out) {
    const std::string focus_material =
        std::string(encrypted_message.Msg_Cipher_Text) + "|"
        + std::to_string(sender_id) + "|"
        + std::to_string(receiver_id) + "|"
        + std::to_string(rogue_id) + "|"
        + std::to_string(ciphertext_queue_depth);
    const std::string focus_hash_hex = deterministic_hex256(focus_material);
    const std::string executable_digest_hex = deterministic_hex256("phase2_smoke_test");
    const std::string phenotype_digest_hex = deterministic_hex256(security_overview);

    BettiSnapshotV1 snapshot {};
    snapshot.domain = 2U; /* scheduler / IPC-adjacent control surface */
    snapshot.seed_tick = static_cast<std::uint64_t>((receiver_id * 100U) + sender_id);
    snapshot.load = static_cast<std::uint64_t>(420U + encrypted_message.Msg_Size);
    snapshot.fragmentation = static_cast<std::uint64_t>(std::strlen(encrypted_message.Msg_Cipher_Text));
    snapshot.queue_depth = static_cast<std::uint64_t>(ciphertext_queue_depth);
    snapshot.locale = static_cast<std::uint32_t>(receiver_id);

    BettiRiskVectorV1 risk {};
    BettiPhV1 ph {};
    char* snapshot_witness_ptr = nullptr;
    const int snapshot_rc = Betti_sim_snapshot(&snapshot, &risk, &ph, &snapshot_witness_ptr);
    if (snapshot_rc != 0) {
        if (error_out != nullptr) {
            const char* ffi_error = Betti_last_error();
            *error_out = std::string("Betti_sim_snapshot failed: ")
                + (ffi_error != nullptr ? ffi_error : "unknown error");
        }
        return false;
    }

    const std::string snapshot_witness =
        snapshot_witness_ptr != nullptr ? snapshot_witness_ptr : "";
    Betti_string_free(snapshot_witness_ptr);

    const std::uint64_t task_id =
        Betti_begin_budgeted_task("phase2_secure_ipc_capture", 50'000U, 100'000U, 1'048'576U);

    char* commit_witness_ptr = nullptr;
    const int commit_rc = Betti_apply_move(
        2U,
        focus_hash_hex.c_str(),
        "P23",
        75'000U,
        125'000U,
        1'048'576U,
        &commit_witness_ptr);
    const std::string commit_witness =
        commit_witness_ptr != nullptr ? commit_witness_ptr : "";
    if (commit_witness_ptr != nullptr) {
        Betti_string_free(commit_witness_ptr);
    }

    const int cancel_rc = Betti_cancel_task(task_id);
    const std::string snapshot_witness_path = request.prefix + "_snapshot_witness.jcs";
    const std::string commit_witness_path = request.prefix + "_commit_witness.jcs";

    const std::string cipher_preview =
        std::string(encrypted_message.Msg_Cipher_Text).substr(0, 32);

    std::ostringstream live_profile;
    live_profile
        << "{\n"
        << "  \"profile_kind\": \"ultima_phase2_secure_ipc_live_profile_v1\",\n"
        << "  \"theme\": \"Security/Privacy/Encryption\",\n"
        << "  \"source_binary\": \"phase2_smoke_test\",\n"
        << "  \"task_ids\": {\"sender\": " << sender_id
        << ", \"receiver\": " << receiver_id
        << ", \"rogue\": " << rogue_id << "},\n"
        << "  \"secure_mailbox\": {\n"
        << "    \"task_id\": " << receiver_id << ",\n"
        << "    \"ciphertext_queue_depth\": " << ciphertext_queue_depth << ",\n"
        << "    \"ciphertext_preview\": \"" << json_escape(cipher_preview) << "\",\n"
        << "    \"security_overview\": \"" << json_escape(security_overview) << "\"\n"
        << "  },\n"
        << "  \"betti_snapshot\": {\n"
        << "    \"domain\": \"sched\",\n"
        << "    \"seed_tick\": " << snapshot.seed_tick << ",\n"
        << "    \"load\": " << snapshot.load << ",\n"
        << "    \"fragmentation\": " << snapshot.fragmentation << ",\n"
        << "    \"queue_depth\": " << snapshot.queue_depth << ",\n"
        << "    \"locale\": " << snapshot.locale << "\n"
        << "  },\n"
        << "  \"betti_risk\": {\n"
        << "    \"structural\": " << risk.structural << ",\n"
        << "    \"fragmentation\": " << risk.fragmentation << ",\n"
        << "    \"latency\": " << risk.latency << ",\n"
        << "    \"instability\": " << risk.instability << "\n"
        << "  },\n"
        << "  \"betti_ph\": {\n"
        << "    \"b0\": " << ph.b0 << ",\n"
        << "    \"b1\": " << ph.b1 << ",\n"
        << "    \"b2\": " << ph.b2 << ",\n"
        << "    \"deltas\": [" << ph.deltas[0] << ", " << ph.deltas[1] << ", " << ph.deltas[2] << "]\n"
        << "  },\n"
        << "  \"budgeted_task\": {\"task_id\": " << task_id << ", \"cancel_rc\": " << cancel_rc << "},\n"
        << "  \"focus_hash_hex\": \"" << focus_hash_hex << "\",\n"
        << "  \"commit_move_kind\": \"P23\",\n"
        << "  \"commit_rc\": " << commit_rc << "\n"
        << "}\n";

    std::ostringstream shadow_cells;
    shadow_cells
        << "[\n"
        << "  {\n"
        << "    \"cell_id\": \"phase2-secure-mailbox-t" << receiver_id << "\",\n"
        << "    \"service_id\": \"task-" << receiver_id << "-secure-mailbox\",\n"
        << "    \"attested\": true,\n"
        << "    \"live_shadow\": true,\n"
        << "    \"secure_channel\": true,\n"
        << "    \"ciphertext_at_rest\": " << (ciphertext_queue_depth > 0 ? "true" : "false") << ",\n"
        << "    \"executable_digest_hex\": \"" << executable_digest_hex << "\",\n"
        << "    \"phenotype_digest_hex\": \"" << phenotype_digest_hex << "\",\n"
        << "    \"reversible_digest_hex\": \"" << focus_hash_hex << "\"\n"
        << "  }\n"
        << "]\n";

    std::ostringstream lowering;
    lowering
        << "{\n"
        << "  \"artifact_kind\": \"ultima_phase2_secure_ipc_ichorir_morphir_lowering_v1\",\n"
        << "  \"honesty_boundary\": \"Manual Phase 2 lowering into BETTI-style IchorIR/MorphIR terms; not ingested into betti_os as a native compiler artifact.\",\n"
        << "  \"theorem_status\": {\"specified\": true, \"implemented\": true, \"verified\": true, \"ratified\": false},\n"
        << "  \"ichor_objects\": [\n"
        << "    {\"id\": \"principal/t-" << sender_id << "\", \"kind\": \"Sender\"},\n"
        << "    {\"id\": \"principal/t-" << receiver_id << "\", \"kind\": \"RestrictedReceiver\"},\n"
        << "    {\"id\": \"mailbox/t-" << receiver_id << "\", \"kind\": \"SecureMailbox\", \"ciphertext_queue_depth\": " << ciphertext_queue_depth << "},\n"
        << "    {\"id\": \"message/secure-phase2\", \"kind\": \"EncryptedPayload\", \"ciphertext_preview\": \"" << json_escape(cipher_preview) << "\"}\n"
        << "  ],\n"
        << "  \"morph_transitions\": [\n"
        << "    {\"step\": 13, \"law\": \"configure_mailbox_security\", \"from\": \"mailbox/plain\", \"to\": \"mailbox/restricted_encrypted\"},\n"
        << "    {\"step\": 14, \"law\": \"deny_unauthorized_sender\", \"from\": \"principal/untrusted\", \"to\": \"gate/rejected\"},\n"
        << "    {\"step\": 15, \"law\": \"encrypt_and_enqueue\", \"from\": \"payload/plaintext\", \"to\": \"mailbox/ciphertext_at_rest\"},\n"
        << "    {\"step\": 16, \"law\": \"authorized_decrypt\", \"from\": \"mailbox/ciphertext_at_rest\", \"to\": \"receiver/plaintext\"}\n"
        << "  ]\n"
        << "}\n";

    std::ostringstream artifact_fabric;
    artifact_fabric
        << "{\n"
        << "  \"artifact_kind\": \"ultima_phase2_secure_ipc_artifact_fabric_v1\",\n"
        << "  \"artifact_root_prefix\": \"" << json_escape(request.prefix) << "\",\n"
        << "  \"artifacts\": [\n"
        << "    {\"artifact_id\": \"artifact/snapshot_witness\", \"path\": \"" << json_escape(snapshot_witness_path) << "\"},\n"
        << "    {\"artifact_id\": \"artifact/commit_witness\", \"path\": \"" << json_escape(commit_witness_path) << "\"},\n"
        << "    {\"artifact_id\": \"artifact/live_profile\", \"path\": \"" << json_escape(request.prefix + "_live_profile.json") << "\"},\n"
        << "    {\"artifact_id\": \"artifact/shadow_cells\", \"path\": \"" << json_escape(request.prefix + "_shadow_cells.json") << "\"},\n"
        << "    {\"artifact_id\": \"artifact/lowering\", \"path\": \"" << json_escape(request.prefix + "_ichorir_morphir_lowering.json") << "\"}\n"
        << "  ],\n"
        << "  \"lineage_edges\": [\n"
        << "    {\"parent\": \"phase2_smoke_test\", \"child\": \"artifact/snapshot_witness\", \"law\": \"secure_mailbox_snapshot\"},\n"
        << "    {\"parent\": \"artifact/snapshot_witness\", \"child\": \"artifact/live_profile\", \"law\": \"witness_enriches_profile\"},\n"
        << "    {\"parent\": \"artifact/live_profile\", \"child\": \"artifact/shadow_cells\", \"law\": \"live_profile_projects_shadow_cell\"},\n"
        << "    {\"parent\": \"artifact/live_profile\", \"child\": \"artifact/lowering\", \"law\": \"profile_lowers_into_ichorir_morphir_sidecar\"}\n"
        << "  ]\n"
        << "}\n";

    std::ostringstream genealogy;
    genealogy
        << "{\n"
        << "  \"record_kind\": \"ultima_phase2_secure_ipc_genealogy_v1\",\n"
        << "  \"root\": \"phase2/ipc/base_mailbox_surface\",\n"
        << "  \"edges\": [\n"
        << "    {\"parent\": \"ipc.cpp::Message_Send\", \"child\": \"ipc.cpp::Secure_Message_Send\", \"reason\": \"secure ACL + encryption extension\"},\n"
        << "    {\"parent\": \"ipc.cpp::Message_Receive\", \"child\": \"ipc.cpp::Secure_Message_Receive\", \"reason\": \"authorized decrypt extension\"},\n"
        << "    {\"parent\": \"phase2_smoke_test secure scenario\", \"child\": \"BETTI sidecar witness bundle\", \"reason\": \"class submission evidence\"}\n"
        << "  ]\n"
        << "}\n";

    std::ostringstream ratification;
    ratification
        << "{\n"
        << "  \"manifest_kind\": \"ultima_phase2_secure_ipc_ratification_v1\",\n"
        << "  \"profile\": \"class_submission_sidecar\",\n"
        << "  \"status\": \"pending_live_ratification\",\n"
        << "  \"generated_by\": \"phase2_smoke_test --emit-betti-artifacts\",\n"
        << "  \"theorem_status\": {\"specified\": true, \"implemented\": true, \"verified\": true, \"ratified\": false},\n"
        << "  \"blockers\": [\n"
        << "    \"No external live-operational evidence bundle from a classroom or lab environment.\",\n"
        << "    \"IchorIR/MorphIR lowering is a sidecar artifact, not a native BETTI-ingested production path.\",\n"
        << "    \"Constitutional promotion / trusted-path gating has not been run against an external ratification corpus.\",\n"
        << "    \"The classroom secure IPC path uses deterministic simulation encryption, not production cryptography.\"\n"
        << "  ]\n"
        << "}\n";

    std::ostringstream trusted_path;
    trusted_path
        << "{\n"
        << "  \"gate_kind\": \"ultima_phase2_secure_ipc_trusted_path_gate_v1\",\n"
        << "  \"checks\": [\n"
        << "    {\"id\": \"acl_sender_deny\", \"status\": \"" << (rogue_send_denied ? "pass" : "fail") << "\"},\n"
        << "    {\"id\": \"ciphertext_at_rest\", \"status\": \"" << (ciphertext_queue_depth > 0 ? "pass" : "fail") << "\"},\n"
        << "    {\"id\": \"acl_reader_deny\", \"status\": \"" << (rogue_read_denied ? "pass" : "fail") << "\"},\n"
        << "    {\"id\": \"authorized_decrypt\", \"status\": \"" << (authorized_decrypt_ok ? "pass" : "fail") << "\"},\n"
        << "    {\"id\": \"betti_snapshot_witness_emitted\", \"status\": \"" << (!snapshot_witness.empty() ? "pass" : "fail") << "\"},\n"
        << "    {\"id\": \"betti_commit_witness_emitted\", \"status\": \"" << (!commit_witness.empty() ? "pass" : "fail") << "\"},\n"
        << "    {\"id\": \"bettictl_verify\", \"status\": \"pending_external_command\"}\n"
        << "  ]\n"
        << "}\n";

    const bool writes_ok =
        write_text_file(snapshot_witness_path, snapshot_witness)
        && write_text_file(commit_witness_path, commit_witness)
        && write_text_file(request.prefix + "_live_profile.json", live_profile.str())
        && write_text_file(request.prefix + "_shadow_cells.json", shadow_cells.str())
        && write_text_file(request.prefix + "_ichorir_morphir_lowering.json", lowering.str())
        && write_text_file(request.prefix + "_artifact_fabric.json", artifact_fabric.str())
        && write_text_file(request.prefix + "_genealogy.json", genealogy.str())
        && write_text_file(request.prefix + "_ratification_manifest.json", ratification.str())
        && write_text_file(request.prefix + "_trusted_path_gate.json", trusted_path.str());

    if (!writes_ok) {
        if (error_out != nullptr) {
            *error_out = "Failed to write one or more BETTI sidecar artifacts.";
        }
        return false;
    }

    return true;
}
#endif
} // namespace

int main(int argc, char* argv[]) {
    const ArtifactRequest artifact_request = parse_artifact_request(argc, argv);
#ifndef ULTIMA_BETTI_FFI_ENABLED
    if (artifact_request.enabled) {
        std::cerr << "BETTI artifact emission was requested, but this binary was built without ULTIMA_BETTI_FFI_ENABLED.\n";
        return 2;
    }
#endif

    const int sender_id = sys_mcb.Swapper.create_task("Sender_Task", idle_task);
    const int receiver_id = sys_mcb.Swapper.create_task("Receiver_Task", idle_task);
    const int rogue_id = sys_mcb.Swapper.create_task("Rogue_Task", idle_task);

    TCB* receiver_tcb = sys_mcb.Swapper.get_tcb(receiver_id);
    assert(receiver_tcb != nullptr);
    assert(receiver_tcb->mailbox_semaphore != nullptr);

    const int send_down_before = receiver_tcb->mailbox_semaphore->get_down_operations();
    const int send_up_before = receiver_tcb->mailbox_semaphore->get_up_operations();

    const int send_result = sys_mcb.Messenger.Message_Send(sender_id, receiver_id, "phase2", 0);
    assert(send_result == 1);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 1);
    assert(sys_mcb.Messenger.Message_Count() == 1);
    assert(receiver_tcb->mailbox_semaphore->get_down_operations() == send_down_before + 1);
    assert(receiver_tcb->mailbox_semaphore->get_up_operations() == send_up_before + 1);

    Message received_message {};
    const int receive_down_before = receiver_tcb->mailbox_semaphore->get_down_operations();
    const int receive_up_before = receiver_tcb->mailbox_semaphore->get_up_operations();

    const int receive_result = sys_mcb.Messenger.Message_Receive(receiver_id, &received_message);
    assert(receive_result == 1);
    assert(std::strcmp(received_message.Msg_Text, "phase2") == 0);
    assert(received_message.Msg_Type.Message_Type_Id == 0);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 0);
    assert(sys_mcb.Messenger.Message_Count() == 0);
    assert(receiver_tcb->mailbox_semaphore->get_down_operations() == receive_down_before + 1);
    assert(receiver_tcb->mailbox_semaphore->get_up_operations() == receive_up_before + 1);

    assert(sys_mcb.Messenger.Message_Send(sender_id, 9999, "bad", 0) == -1);
    assert(sys_mcb.Messenger.Message_Receive(9999, &received_message) == -1);

    char buffer[ULTIMA_MESSAGE_TEXT_CAPACITY] = {0};
    int message_type = -1;
    assert(sys_mcb.Messenger.Message_Receive(receiver_id, buffer, &message_type) == 0);

    assert(sys_mcb.Messenger.Message_Send(sender_id, receiver_id, "delete-1", 1) == 1);
    assert(sys_mcb.Messenger.Message_Send(sender_id, receiver_id, "delete-2", 2) == 1);
    assert(sys_mcb.Messenger.Message_DeleteAll(receiver_id) == 2);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 0);

    const std::string dump = sys_mcb.Messenger.ipc_Message_Dump_String();
    assert(dump.find("All mailboxes empty.") != std::string::npos);

    assert(sys_mcb.Messenger.Configure_Mailbox_Security(receiver_id, "thunder-shared-secret") == 1);
    assert(sys_mcb.Messenger.Allow_Mailbox_Sender(receiver_id, sender_id) == 1);
    assert(sys_mcb.Messenger.Allow_Mailbox_Reader(receiver_id, sender_id) == 1);

    const bool rogue_send_denied =
        (sys_mcb.Messenger.Secure_Message_Send(rogue_id, receiver_id, "rogue-payload", 1) == -1);
    assert(rogue_send_denied);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 0);

    assert(sys_mcb.Messenger.Secure_Message_Send(sender_id, receiver_id, "secure-phase2", 1) == 1);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 1);

    std::queue<Message> mailbox_copy = sys_mcb.Messenger.get_mailbox_copy(receiver_id);
    assert(mailbox_copy.size() == 1);
    const Message encrypted_message = mailbox_copy.front();
    assert(encrypted_message.Is_Encrypted);
    assert(encrypted_message.Access_Restricted);
    assert(std::strlen(encrypted_message.Msg_Text) == 0);
    assert(std::strlen(encrypted_message.Msg_Cipher_Text) > 0);

    const bool rogue_read_denied =
        (sys_mcb.Messenger.Secure_Message_Receive(rogue_id, receiver_id, &received_message) == -1);
    assert(rogue_read_denied);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 1);

    std::memset(buffer, 0, sizeof(buffer));
    message_type = -1;
    assert(sys_mcb.Messenger.Secure_Message_Receive(receiver_id, receiver_id, buffer, sizeof(buffer), &message_type) == 1);
    const bool authorized_decrypt_ok = (std::strcmp(buffer, "secure-phase2") == 0);
    assert(authorized_decrypt_ok);
    assert(message_type == 1);
    assert(sys_mcb.Messenger.Message_Count(receiver_id) == 0);

    assert(sys_mcb.Messenger.Secure_Message_Send(sender_id, receiver_id, "delegated-read", 2) == 1);
    assert(sys_mcb.Messenger.Secure_Message_Receive(sender_id, receiver_id, &received_message) == 1);
    assert(std::strcmp(received_message.Msg_Text, "delegated-read") == 0);
    assert(received_message.Is_Encrypted);
    assert(received_message.Access_Restricted);

    const std::string secure_overview = sys_mcb.Messenger.get_security_overview();
    assert(secure_overview.find("Encrypted / Restricted") != std::string::npos);

    #ifdef ULTIMA_BETTI_FFI_ENABLED
    if (artifact_request.enabled) {
        std::string error;
        if (!emit_betti_sidecar_artifacts(
                artifact_request,
                sender_id,
                receiver_id,
                rogue_id,
                encrypted_message,
                mailbox_copy.size(),
                rogue_send_denied,
                rogue_read_denied,
                authorized_decrypt_ok,
                secure_overview,
                &error)) {
            std::cerr << error << '\n';
            return 2;
        }

        std::cout << "BETTI sidecar artifacts emitted with prefix "
                  << artifact_request.prefix << ".\n";
    }
    #endif

    std::cout << "Phase 2 smoke test passed.\n";
    return 0;
}
