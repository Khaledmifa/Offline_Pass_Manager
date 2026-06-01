#include "Language.h"
Lang Language::_lang = Lang::EN;

static const char* EN[] = {"OK","Cancel","Back","Next","Yes","No","Save","Delete","Locked","Enter PIN","Scan Finger","Wrong PIN","No Match","Auth Failed","PIN Mismatch","Fingerprint Saved","Credentials","PW Generator","Settings","Backup","Lock","User + Password","Username Only","Password Only","Typing...","Set PIN","Confirm PIN","Enroll Fingerprint","Add Another?","Backup Freq.","Setup Done!","Place Finger","Lift & Replace","Language","HID Mode","Auto-Lock","Fingerprints","Backup Now","About","USB Keyboard","BLE Keyboard","Connecting...","Connected","WiFi Failed","Backup Sent","Backup Failed","Battery Low","Battery Critical"};
static const char* AR[] = {"موافق","إلغاء","رجوع","التالي","نعم","لا","حفظ","حذف","مقفل","أدخل PIN","امسح البصمة","رقم خاطئ","لا تطابق","فشل التحقق","PIN غير متطابق","تم حفظ البصمة","كلمات المرور","توليد كلمة","الإعدادات","نسخة احتياطية","قفل","اسم + كلمة","الاسم فقط","الكلمة فقط","جاري الإدخال...","ضبط PIN","تأكيد PIN","تسجيل بصمة","إضافة بصمة؟","تكرار النسخ","اكتمل الإعداد!","ضع إصبعك","ارفع واعد","اللغة","وضع HID","القفل التلقائي","البصمات","نسخ الآن","حول","USB لوحة","BLE لوحة","جاري الاتصال...","متصل","فشل WiFi","تم الإرسال","فشل النسخ","بطارية منخفضة","بطارية حرجة"};
static const char* DE[] = {"OK","Abbrechen","Zurück","Weiter","Ja","Nein","Speichern","Löschen","Gesperrt","PIN eingeben","Finger scannen","Falsche PIN","Kein Treffer","Fehler","PIN falsch","FP gespeichert","Passwörter","PW Generator","Einstellungen","Backup","Sperren","Benutzer+PW","Nur Benutzer","Nur Passwort","Eingabe...","PIN setzen","PIN bestätigen","FP registrieren","Weiteren?","Backup-Häufigkeit","Einrichtung OK!","Finger auflegen","Heben+auflegen","Sprache","HID-Modus","Auto-Sperre","Fingerabdrücke","Jetzt sichern","Info","USB Tastatur","BLE Tastatur","Verbinde...","Verbunden","WLAN Fehler","Backup gesendet","Backup Fehler","Akku schwach","Akku kritisch"};
static const char* FR[] = {"OK","Annuler","Retour","Suivant","Oui","Non","Sauv.","Suppr.","Verrouillé","Entrer PIN","Scanner doigt","PIN erroné","Pas de match","Échec auth","PIN différents","Empreinte OK","Identifiants","Gén. MDP","Paramètres","Sauvegarde","Verrouiller","Utilisateur+MDP","Utilisateur","MDP seul","Saisie...","Définir PIN","Confirmer PIN","Enregistrer","Autre?","Fréquence","Config OK!","Posez le doigt","Soulevez","Langue","Mode HID","Verrou auto","Empreintes","Sauv. maintenant","À propos","Clavier USB","Clavier BLE","Connexion...","Connecté","Échec WiFi","Sauvegarde OK","Échec sauvegarde","Batterie faible","Batterie critique"};
static const char* TR[] = {"Tamam","İptal","Geri","İleri","Evet","Hayır","Kaydet","Sil","Kilitli","PIN gir","Parmak tara","Yanlış PIN","Eşleşme yok","Hata","PIN uyuşmuyor","Parmak kaydedildi","Kimlik Bilgileri","Şifre Üret","Ayarlar","Yedekle","Kilitle","Kullanıcı+Şifre","Yalnızca kul.","Yalnızca şifre","Yazılıyor...","PIN ayarla","PIN doğrula","Parmak kaydet","Başka ekle?","Yedek sıklığı","Kurulum tamam!","Parmağı koy","Kaldır+koy","Dil","HID Modu","Oto kilit","Parmak izleri","Şimdi yedekle","Hakkında","USB Klavye","BLE Klavye","Bağlanıyor...","Bağlı","WiFi hatası","Yedek gönderildi","Yedek başarısız","Pil düşük","Pil kritik"};
static const char* ZH[] = {"确认","取消","返回","下一步","是","否","保存","删除","已锁定","输入PIN","扫描指纹","PIN错误","不匹配","认证失败","PIN不匹配","指纹已保存","密码库","密码生成","设置","备份","锁定","用户名+密码","仅用户名","仅密码","输入中...","设置PIN","确认PIN","录入指纹","添加更多?","备份频率","设置完成!","放置手指","抬起再放","语言","HID模式","自动锁定","指纹","立即备份","关于","USB键盘","蓝牙键盘","连接中...","已连接","WiFi失败","备份已发送","备份失败","电量低","电量危急"};

void Language::setLanguage(Lang l) { _lang = l; }
Lang Language::getLanguage()       { return _lang; }
bool Language::isRTL()             { return _lang == Lang::AR; }
const char* Language::get(Str key) {
  uint8_t k = (uint8_t)key;
  switch (_lang) {
    case Lang::AR: return AR[k];
    case Lang::DE: return DE[k];
    case Lang::FR: return FR[k];
    case Lang::TR: return TR[k];
    case Lang::ZH: return ZH[k];
    default:       return EN[k];
  }
}
