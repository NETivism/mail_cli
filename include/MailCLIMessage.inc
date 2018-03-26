<?php

class MailCLIMessage {

  /**
   * Recipient's email address.
   *
   * @var array
   */
  protected $to;

  /**
   * Sender's email address.
   *
   * @var string|array
   */
  protected $from;

  /**
   * Mail subject.
   *
   * @var string
   */
  protected $subject;

  /**
   * Mail body.
   *
   * @var string
   */
  protected $message;

  /**
   * Set the message body.
   *
   * @param string $message
   *
   * @return $this
   */
  public function message($message) {
    $this->message = $message;

    return $this;
  }

  /**
   * Set the subject.
   *
   * @param string $subject
   *
   * @return $this
   */
  public function subject($subject) {
    $this->subject = $subject;

    return $this;
  }

  /**
   * Set the sender's email.
   *
   * @param string $from
   *
   * @return $this
   */
  public function from($from) {
    $this->from = $from;

    return $this;
  }

  /**
   * Set the recipient's email.
   *
   * @param string|array $to
   *
   * @return $this
   */
  public function to($to) {
    $this->to = $to;
    if (!is_array($this->to)) {
      $this->to = explode(',', $this->to);
    }

    return $this;
  }

  /**
   * Send the mail now.
   *
   * @return boolean
   * @throws MailCLIException
   */
  public function send() {
    global $user;

    $this->validate();

    $params = [
      'subject' => $this->subject,
      'body' => $this->message,
    ];

    if ($this->from) {
      $params['from'] = $this->from;
    }

    $mail = drupal_mail('mail_cli', 'notice', implode(',', $this->to), user_preferred_language($user), $params);

    if(!$mail['result']) {
      throw new MailCLIException('Unable to send email. Please see errors above.');
    }

    return $mail['result'];
  }

  /**
   * Let the queue handle the mail later.
   *
   * @return boolean
   * @throws MailCLIException
   */
  public function queue() {
    $this->validate();
    return TRUE;
  }

  /**
   * Print the message to the log.
   *
   * @params Print message to a file.
   *
   * @return boolean
   * @throws MailCLIException
   */
  public function log($file = 'php://stdout') {
    $this->validate();

    return TRUE;
  }

  /**
   * Send the message later with cron.
   *
   * @return boolean
   * @throws MailCLIException|Exception
   */
  public function later() {
    $this->validate();

    return db_insert('mail_cli_messages')->fields([
      'mail_object' => base64_encode(serialize($this)),
      'created_at' => time(),
    ])->execute();
  }

  /**
   * @return string
   */
  public function getMessage() {
    return $this->message;
  }

  /**
   * @return string
   */
  public function getSubject() {
    return $this->subject;
  }

  /**
   * @return array
   */
  public function getTo() {
    return is_array($this->to) ? $this->to : [$this->to];
  }

  /**
   * @return string
   */
  public function getFrom() {
    return $this->from;
  }

  /**
   * Validate the request before sending or queueing.
   *
   * @throws MailCLIException
   */
  protected function validate() {
    $errors = [];
    if (empty($this->to)) {
      $errors[] = 'Parameter to is required';
    }

    if (empty($this->subject)) {
      $errors[] = 'Parameter subject is required';
    }

    if (empty($this->message)) {
      $errors[] = 'Parameter message is required';
    }

    if (!empty($errors)) {
      throw new MailCLIException(implode("\n", $errors));
    }
  }
}
