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

  $commands['mail-process'] = [
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

  if (strstr($to, ',') !== FALSE) {
    $to = explode(',', $to);
  }
  else {
    $to = [$to];
  }

  foreach ($to as $one) {
    if (filter_var($one, FILTER_VALIDATE_EMAIL) === FALSE) {
      drush_log("$one is an invalid email address!", 'error');
      return;
    }
  }

  if (!$message) {
    $message = file_get_contents('php://stdin');
  }

  try {
    $mail = (new MailCLIMessage())->to($to)
      ->subject($subject)
      ->message($message);

    if (drush_get_option('later', FALSE)) {
      $mail->later();
      drush_log('Email saved and will be sent later.', 'ok');
    }
    else {
      drush_print('Sending email. Please wait. This may take a few moments.');
      $mail->send();
      drush_log('Email sent successfully.', 'ok');
    }
  } catch (MailCLIException $exception) {
    drush_log('An error occurred why sending the email.', 'error');
    drush_log($exception->getMessage(), 'error');
  }
}

/**
 * Send all mail that has been marked to be sent later.
 *
 * @throws \Exception
 */
function drush_mail_cli_mail_process() {
  $mailObjects = db_select('mail_cli_messages', 'MCM')
    ->fields('MCM', ['id', 'mail_object'])
    ->condition('processed', 0)
    ->execute()
    ->fetchAll();

  foreach ($mailObjects as $mailObject) {
    /** @var \MailCLIMessage $mail */
    $mail = unserialize(base64_decode($mailObject->mailObject));
    try {
      $mail->send();
      drush_log('Message "' . $mail->getSubject() . '" has been sent to "' . $mail->getTo() . '".', 'ok');
      db_update('mail_cli_messages')
        ->fields(['processed' => 1])
        ->condition('id', $mailObject->id)
        ->execute();
    } catch (MailCLIException $exception) {
      drush_log($exception->getMessage(), 'error');
      return;
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
    ->fields('MCM', ['id', 'mail_object', 'created_at'])
    ->condition('processed', 0)
    ->execute()
    ->fetchAll();

  $messages = [];
  $ids = [];

  drush_print("Preparing messages");
  // Concatenate all messages per recipient.
  foreach ($mailObjects as $mailObject) {
    /** @var \MailCLIMessage $mail */
    $mail = unserialize(base64_decode($mailObject->mail_object));
    $time = $mailObject->created_at;
    $recipients = $mail->getTo();
    // For each recipient in the list, add the message to the body.
    foreach ($recipients as $recipient) {
      if (!isset($messages[$recipient])) {
        $messages[$recipient] = '';
      }
      $messages[$recipient] .= mail_cli_make_concatenated_message($mail, $time);
      $ids[$recipient][] = $mailObject->id;
    }
  }

  drush_print("Found " . count($messages) . " messages");

  // Send all messages
  $date = date('Y-m-d H:i:s');
  foreach ($messages as $recipient => $message) {
    try {
      drush_print("Sending messages to $recipient");
      (new MailCLIMessage())
        ->to($recipient)
        ->subject("Concatenated messages $date")
        ->message($message)
        ->send();
      drush_log("Message to $recipient has been sent successfully", 'ok');
      db_update('mail_cli_messages')
        ->fields(['processed' => 1])
        ->condition('id', $ids[$recipient], 'IN')
        ->execute();
    } catch (MailCLIException $exception) {
      drush_log($exception->getMessage(), 'error');
      return;
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
