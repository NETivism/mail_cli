<?php

/**
 * Implements hook_drush_command.
 *
 * @return array
 */
function mail_cli_drush_command() {
  $commands = [];

  $commands['send-mail'] = [
    'description' => 'Send an email using the command line',
    'aliases' => ['mail'],
    'options' => [
      'to' => 'Recipient\'s email address',
      'subject' => 'Message subject',
      'message' => '[Optional] The body of the email. If not provided, the message will be read from STDIN.',
      'later' => '[Optional] Whether to save the message to be processed later',
    ],
    'examples' => [
      'drush send-mail --to=test@example.com --message="Hello,\nI am testing"' => 'Set a message as an option',
      'drush send-mail --to=test@example.com' => 'Read the message from STDIN.',
    ],
  ];

  $commands['process-mail'] = [
    'description' => 'Send all messages that have been stored for later.',
    'examples' => [
      'drush process-mail' => 'Processes all mail',
    ],
  ];

  $commands['mail-process-concat'] = [
    'description' => 'Send all messages that have been stored for later in a single email per recipient',
    'aliases' => ['mail-pc'],
    'options' => [
      'to' => 'Specifies the recepient\'s email. You can provide a comma separated list of email addresses',
    ],
    'examples' => [
      'drush pmc --to=test1@example.com,test2@example.com' => 'Processes all mail',
    ],
  ];

  return $commands;
}

/**
 * Implements drush_HOOK_COMMAND
 *
 * @throws Exception
 */
function drush_mail_cli_send_mail() {
  $to = drush_get_option('to');
  $subject = drush_get_option('subject');
  $message = drush_get_option('message');

  if (!$to) {
    drush_log('Please provide the recipient\'s email using the --to option.', 'error');
    return;
  }

  if (!$subject) {
    drush_log('Please provide a subject using the --subject option.', 'error');
    return;
  }

  if (filter_var($to, FILTER_VALIDATE_EMAIL) === FALSE) {
    drush_log('Please provide a valid recipient\'s email address.', 'error');
    return;
  }

  if (!$message) {
    $message = file_get_contents('php://stdin');
  }

  drush_print('Sending email. Please wait. This may take a few moments.');

  try {
    $mail = (new MailCLIMessage())->to($to)
      ->subject($subject)
      ->message($message);

    if (drush_get_option('later', FALSE)) {
      $mail->later();
    }
    else {
      $mail->send();
    }
  } catch (MailCLIException $exception) {
    drush_log('An error occurred why sending the email.', 'error');
    drush_log($exception->getMessage(), 'error');
    return;
  }

  drush_print('Email sent successfully.');
}

/**
 * Send all mail that has been marked to be sent later.
 *
 * @throws \Exception
 */
function drush_mail_cli_process_mail() {
  $mailObjects = db_select('mail_cli_messages', 'MCM')
    ->fields(['mail_object'])
    ->execute()
    ->fetchAll();

  foreach ($mailObjects as $mailObject) {
    /** @var \MailCLIMessage $mail */
    $mail = unserialize($mailObject);
    try {
      $mail->send();
      drush_print('Message "' . $mail->getSubject() . '" has been sent to "' . $mail->getTo() . '".');
    } catch (MailCLIException $exception) {
      drush_log($exception->getMessage(), 'error');
    }
  }
}

/**
 * Sends contacted messages per recipient.
 *
 * @throws Exception
 */
function drush_mail_cli_mail_process_concat() {
  $mailObjects = db_select('mail_cli_messages', 'MCM')
    ->fields(['mail_object', 'created_at'])
    ->fetchAll();

  $messages = [];

  drush_print("Preparing messages");
  // Concatenate all messages per recipient.
  foreach ($mailObjects as $mailObject) {
    /** @var \MailCLIMessage $mail */
    $mail = unserialize($mailObject->mail_object);
    $time = $mailObject->created_at;
    $recipients = $mail->getTo();
    // For each recipient in the list, add the message to the body.
    foreach ($recipients as $recipient) {
      if (!isset($messages[$recipient])) {
        $messages[$recipient] = '';
      }
      $messages[$recipient] .= mail_cli_make_concatenated_message($mail, $time);
    }
  }

  // Send all messages
  $date = date('Y-m-d H:i:s');
  foreach ($messages as $recipient => $message) {
    drush_print("Sending messages to $recipient");
    try {
      (new MailCLIMessage())
        ->to($recipient)
        ->subject("Concatenated messages $date")
        ->message($message)
        ->send();
    } catch (MailCLIException $exception) {
      drush_log($exception->getMessage(), 'error');
    }
  }
}

/**
 * Create a message for use in the concatenated option.
 *
 * @param \MailCLIMessage $mail
 * @param int $time
 *
 * @return string
 */
function mail_cli_make_concatenated_message($mail, $time) {
  $date = date('Y-m-d H:i:s', $time);
  $cat = "Sent at $date\n";
  $cat .= "Subject: " . $mail->getSubject() . "\n";
  $cat .= $mail->getMessage() . "\n";

  return $cat;
}
