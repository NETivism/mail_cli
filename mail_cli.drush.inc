<?php

function mail_cli_drush_command()
{
    $commands = [];

    $commands['send-mail'] = [
        'description' => 'Send an email using the command line',
        'aliases' => ['mail'],
        'options' => [
            'to' => 'Recipient\'s email address',
            'subject' => 'Message subject',
            'message' => '[Optional] The body of the email. If not provided, the message will be read from STDIN.'
        ],
        'examples' => [
            'drush send-mail --to=test@example.com --message="Hello,\nI am testing"' => 'Set a message as an option',
            'drush send-mail --to=test@example.com' => 'Read the message from STDIN.',
        ]
    ];

    return $commands;
}

function drush_mail_cli_send_mail()
{
    global $user;

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
        $message .= file_get_contents('php://stdin');
    }

    try {
        $mail = drupal_mail('mail_cli', 'notice', $to, user_preferred_language($user), [
            'subject' => $subject,
            'body' => $message
        ]);
    } catch (Exception $exception) {
        drush_log('An error occurred why sending the email.', 'error');
        drush_log($exception->getMessage(), 'error');
    }
}
