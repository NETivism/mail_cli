## Send mail via drush

This module allows you to use the command line to send emails through the Drupal
system. It utilizes Drupal's mail system to hook into any mail module's API
such as Mailgun.

## Installation
Download the module into `sites/all/modules` then enable it using the admin pages
or by running `drush en mail_cli`.

## Usage

**Sending mail**

```bash
# Example with inline message
# mail is alias to send-mail
drush mail --to=test@example.com --subject="The subject" --message="My message here"

# Example message read from STDIN
drush mail --to=test@example.com --subject="My subject" < my_message_file.txt 

# Save message to get processed later
drush mail --to=test@example.com --subject="The subject" --message="My message here" --later
```

**Processing Saved Emails**

Saved emails can be processed using one of the following commands
```bash
# Send a each saved message individually
drush mail-process

# Concatenate all messages into one email per recipient
# mail-pc is an alias to mail-process-concat
drush mail-pc
```

This command can be added to crontab and executed every certain interval of time. For example, to
send emails once a day at 10 PM, add the following line to crontab:
```
# Where /var/www/html is your drupal root directory
 0 22 * * * cd /var/www/html && drush mail-pc
```

## License
This module is licensed under GPLv3.
