<?php
// $Id: node_resource.inc,v 1.1 2010/03/08 20:01:41 hugowetterberg Exp $

/**
 * Get user details.
 *
 * @param $uid
 *   UID of the user to be loaded.
 *
 * @return
 *   A user object.
 *
 * @see user_load()
 */
function _user_resource_retrieve($uid) {
  $account = user_load($uid);
  if (empty($account)) {
    return services_error(t('There is no user with such ID.'), 404);
  }

  // Everything went right.
  return $account;
}

/**
 * Create a new user.
 *
 * This function uses drupal_execute() and as such exepects all input to match 
 * the submitting form in question.
 *
 * @param $account
 *   A object containing account information. The object must at minimum
 *   contain username, password and email address.
 *
 * @return
 *   The user object of the newly created user.
 */
function _user_resource_create($account) {
  // Load the required includes for saving profile information
  // with drupal_execute().
  module_load_include('inc', 'user', 'user.pages');

  // Any logged in user is by default authenticated,
  // and leaving this role set in the user's roles array
  // causes big problems because of a FAPI hack that controls
  // this checkbox on the user create and edit form (and thus
  // causes problems with drupal_execute()). Therefore we just
  // force it to 0 here.
  if (isset($account->roles[2])) {
    $account->roles[2] = 0;
  }

  // register a new user
  $form_state['values'] = (array) $account;
  $form_state['values']['pass'] = array(
    'pass1' => $account->pass,
    'pass2' => $account->pass,
  );
  $form_state['values']['op'] = t('Create new account');
  
  $ret = drupal_execute('user_register', $form_state);

  // Error if needed.
  if ($errors = form_get_errors()) {
    return services_error(implode("\n", $errors), 401);
  }
  else {
    return $form_state['user'];
  }
}

/**
 * Update an existing user.
 *
 * This function uses drupal_execute() and as such exepects all input to match 
 * the submitting form in question.
 *
 * @param $account
 *   A properly formatted user object.
 *
 * @return
 *   The modified user object.
 */
function _user_resource_update($account) {
  // Load the required includes for saving profile information
  // with drupal_execute().
  module_load_include('inc', 'user', 'user.pages');

  // Any logged in user is by default authenticated,
  // and leaving this role set in the user's roles array
  // causes big problems because of a FAPI hack that controls
  // this checkbox on the user create and edit form (and thus
  // causes problems with drupal_execute()). Therefore we just
  // force it to 0 here.
  if (isset($account->roles[2])) {
    $account->roles[2] = 0;
  }

  // If a profile category was passed in, use it. Otherwise default
  // to 'account' (for saving core user data.)
  $category = 'account';
  if (isset($account->category)) {
    $category = $account->category;
    unset($account->category);
  }
  
  // Drop any passed in values into the $account var. Anything
  // unused by the form just gets ignored.
  foreach ($account as $key => $value) {
    $form_state['values'][$key] = $value;
  }

  $form_state['values']['op'] = 'Save';
  $form_state['values']['_category'] = $category;
  $form_state['values']['_account'] = $account;

  $ret = drupal_execute('user_profile_form', $form_state, $account, $category);

  // Error if needed.
  if ($errors = form_get_errors()) {
    return services_error(implode("\n", $errors), 401);
  }
  else {
    return $account;
  }
}

/**
 * Delete a user.
 *
 * @param $uid
 *   UID of the user to be deleted.
 *
 * @see user_delete()
 */
function _user_resource_delete($uid) {
  $account = user_load($uid);
  if (empty($account)) {
    return services_error(t('There is no user with such ID.'), 404);
  }
  user_delete($account, $uid);

  // Everything went right.
  return TRUE;
}

/**
 * Login a user using the specified credentials.
 *
 * Note this will transfer a plaintext password.
 *
 * @param $username
 *   Username to be logged in.
 * @param $password
 *   Password, must be plain text and not hashed.
 *
 * @return
 *   A valid session object.
 */
function _user_resource_login($username, $password) {
  global $user;

  if ($user->uid) {
    // user is already logged in
    return services_error(t('Already logged in as !user.', array('!user' => $user->name)), 406);
  }

  $user = user_authenticate(array('name' => $username, 'pass' => $password));

  if ($user->uid) {
    // Regenerate the session ID to prevent against session fixation attacks.
    sess_regenerate();
    module_invoke_all('user', 'login', NULL, $user);

    $return = new stdClass();
    $return->sessid = session_id();
    $return->user = $user;

    return $return;
  }
  session_destroy();
  return services_error(t('Wrong username or password.'), 401);
}

/**
 * Logout the current user.
 */
function user_service_logout() {
  global $user;

  if (!$user->uid) {
    // User is not logged in
    return services_error(t('User is not logged in.'), 406);
  }

  watchdog('user', 'Session closed for %name.', array('%name' => theme('placeholder', $user->name)));

  // Destroy the current session:
  session_destroy();
  module_invoke_all('user', 'logout', NULL, $user);

  // Load the anonymous user
  $user = drupal_anonymous_user();

  return TRUE;
}
