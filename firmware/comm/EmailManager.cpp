#include "EmailManager.h"
#include "../config.h"
#include <ESP_Mail_Client.h>

static SMTPSession smtp;

static bool _sendVault(const NetworkConfig& cfg,
                        const uint8_t* blob, size_t blobLen) {
  Session_Config sc;
  sc.server.host_name = cfg.smtp_server;
  sc.server.port      = cfg.smtp_port;
  sc.login.email      = cfg.smtp_email;
  sc.login.password   = cfg.smtp_pass;
  sc.login.user_domain = "";
  sc.secure.startTLS  = (cfg.smtp_port == 587);

  SMTP_Message msg;
  msg.sender.name  = "Password Manager";
  msg.sender.email = cfg.smtp_email;

  // Two recipients
  msg.addRecipient("User",  cfg.to_user);
  msg.addRecipient("Admin", cfg.to_admin);

  // Build subject with date
  char subj[64];
  snprintf(subj, sizeof(subj), "PM Backup — Device %06X",
           (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF));
  msg.subject = subj;
  msg.text.content = "Encrypted vault backup attached. "
                     "Contents are AES-256 encrypted and require your device PIN to decrypt.";

  // Attach vault blob
  SMTP_Attachment att;
  att.descr.filename    = "vault.enc";
  att.descr.mime        = "application/octet-stream";
  att.blob.data         = blob;
  att.blob.size         = blobLen;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  msg.addAttachment(att);

  smtp.debug(0);
  if (!smtp.connect(&sc)) return false;
  bool ok = MailClient.sendMail(&smtp, &msg, true);
  smtp.closeSession();
  return ok;
}

bool EmailManager::testConnection(const NetworkConfig& cfg) {
  // Send a minimal test email
  const uint8_t dummy[] = {'T','E','S','T'};
  return _sendVault(cfg, dummy, 4);
}

bool EmailManager::sendBackup() {
  NetworkConfig cfg;
  if (!Storage::loadNetwork(cfg)) return false;
  return sendBackupWith(cfg);
}

bool EmailManager::sendBackupWith(const NetworkConfig& cfg) {
  static uint8_t buf[65536];
  size_t len = 0;
  if (!Storage::readVaultBlob(buf, len, sizeof(buf))) return false;
  return _sendVault(cfg, buf, len);
}
