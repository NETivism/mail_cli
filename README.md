## Send mail via drush

This module allows you to use the command line to send emails through the Drupal
system. It utilizes Drupal's mail system to hook into any mail module's API
such as Mailgun.

## Installation
Download the module into `sites/all/modules` then enable it using the admin pages
or by running `drush en mail_cli`.

## Usage
```bash
# Example with inline message
# mail is alias to send-mail
drush mail --to=test@example.com --subject="The subject" --message="My message here"

# Example message read from STDIN
drush mail --to=test@example.com --subject="My subject" < my_message_file.txt 
```

## License
This module is licensed under GPLv3.
