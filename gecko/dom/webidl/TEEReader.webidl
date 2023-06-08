/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

enum TEEResult {
  "Success",                        // Intended use case achieved.
  "UserSetting",                    // User requested REE OS App Setting menu.
  "UserExit",                       // Exit from TEE application based on user request.
  "ChangeCard",                     // User requested to change card.
  "UserSelectQRUPI"                 // User selected QR Code / unified payment interface.
};

enum TEEError {
  "PinCreationFailed",              // Failed to store PIN.
  "WrongPin",                       // Error due to wrong PIN.
  "PinChangeFailed",                // Failed to change PIN.
  "CardBlocked",                    // Enter wroing PIN for more than 3 times,
                                    // user needs to change PIN.
  "CardPinBlocked",                 // Enter wrong PIN for more than 5 times,
                                    // PIN is blocked. User need to call customer
                                    // care to RESET of PIN.
  "PaymentFailed",                  // Payment transaction failed.
  "ReceiveFailed",                  // Receive money transaction failed.
  "WrongTransactionID",             // Wrong transaction account ID.
  "MissingTransactionID",           // No valied transaction ID.
  "SecureIndicatorSelectionFailed", // Secure indicator selection failed.
  "GenericError"                    // Generic failures.
};

enum TEEApplication {
  "PinRegistration",                // Onboarding, first time PIN creation.
  "PinChange",                      // Change stored PIN in SE.
  "SecureIndicatorUpdate",          // Select secured indicator to be used.
  "InitiatePayment",                // Initiate payment.
  "ReceivePayment"                  // Initiate receive of payment.
};


// Dictionary that represents an TEE request.
dictionary TEERequest {
  // Invoked application running in trusted execute environment.
  required TEEApplication application;

  // Sequence of octets.
  sequence<octet>? data = null;
};

[Func="Navigator::HasTEEReaderSupport",
 CheckAnyPermissions="teereader-manage",
 AvailableIn="CertifiedApps"]
interface TEEResponse {
  readonly attribute TEEResult result;

  // The response's data field bytes
  [Cached, Pure] readonly attribute sequence<octet>? data;

};

enum TrustedServiceType {
  "Payment"
};

[Func="Navigator::HasTEEReaderSupport",
 CheckAnyPermissions="teereader-manage",
 AvailableIn="CertifiedApps"]
interface TEEReader {

  // 'true' if a TEE is present
  readonly attribute boolean isPresent;

  // Type of trusted service.
  readonly attribute TrustedServiceType serviceType;

  /**
   * Opens a session with the TA Secure Element.
   * Note that a reader may have several opened sessions.
   *
   * @return If the operation is successful the promise is resolved with an instance of SESession.
   */
  [Throws]
  Promise<TEESession> openSession(TrustedServiceType? type);

  /**
   * Closes all sessions associated with this Reader.
   *
   */
  [Throws]
  Promise<void> closeAll();
};

[Func="Navigator::HasTEEReaderSupport",
 CheckAnyPermissions="teereader-manage",
 AvailableIn="CertifiedApps"]
interface TEESession {

  // 'reader' that provides this session
  readonly attribute TEEReader reader;

  // Type of trusted application.
  readonly attribute TEEApplication appType;

  // Status of current session
  readonly attribute boolean isClosed;

  /**
   * Invoke an application in trusted execute environment provided by
   * specific trusted service.
   *
   * @param request
   *     TEERequest to be sent to trusted execute environment.
   *
   * @return If success, the promise is resolved with the new created
   * TEEResponse object. Otherwise, rejected with the error of type 'TEEError'.
   */
  [Throws]
  Promise<TEEResponse> invoke(TEERequest request);

  /**
   * Abort the invoked trusted execute environment usecase
   * if rich executed environment is required to execute higher priority event.
   *
   */
  [Throws]
  Promise<void> abort();

  /**
   * Close the active session.
   *
   */
  [Throws]
  Promise<void> close();
};


