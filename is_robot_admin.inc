<?php

/**
 * @file is_robot_admin.inc
 * Is Robot? administrative functions file.
 */

/**
 * The administrative settings for is_robot
 */
function is_robot_settings_form($form, &$form_state) {
  $form['is_robot_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('General Settings'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['is_robot_settings']['is_robot_append_hook_is_robot_robots_alter'] = array(
    '#title' => t('Append or replace values in database with hook values'),
    '#type' => 'select',
    '#options' => array(
      'append' => t('Append'),
      'replace' => t('Replace'),
    ),
    '#default_value' => variable_get('is_robot_append_hook_is_robot_robots_alter', 'append'),
  );
  $form['is_robot_settings']['is_robot_cron_frequency'] = array(
    '#title' => t('How often to run the cron to update the robots list'),
    '#type' => 'select',
    '#options' => array(
      '82400' => t('Daily'),
      '604000' => t('Weekly'),
      '2592000' => t('Every 30 Days'),
    ),
    '#default_value' => variable_get('is_robot_cron_frequency', '604000'),
  );
  return system_settings_form($form);
}

/**
 * List the custom added robots from our database and via hook. 
 *
 * Obviously the hook ones cannot be modified
 */
function is_robot_additional_items() {
  $i = 0;
  $robots = array();
  $replace = array();
  $rows = array();
  $header = array(t('Machine Name'), t('User Agent'), t('Source'), t('Options'));
  
  // Assign all of our databased robots manually entered into the robots array
  $result = db_query('SELECT * FROM {is_robot} WHERE is_manual = 1 ORDER BY robot_id ASC');
  while ($data = $result->fetchObject()) {
    $robots[] = $data;
  }
  
  // Get the robots in hooks. We ignore the replace option here because we're just listing
  // and not acting on them. Implement any hooks to module_is_robot_robots_alter.
  drupal_alter('is_robot_robots', $robots, $replace);
  
  foreach($robots as $robot) {
    $robot = (array) $robot;
  
    //@@TODO: Fix this at some point in our documentation and hooks
    if (!empty($robot['robot-id'])) {
      $robot['robot_id'] = $robot['robot-id'];
    }
      
    $which = (isset($robot['is_manual'])) == 1 ? 'DATABASE' : 'HOOK';
    
    $row = array();
    $row[] = $robot['robot_id'];
    $row[] = $robot['user_agent'];
    $row[] = $which;
    if ($which == 'DATABASE') {
      $row[] = l(t('Edit'), 'admin/config/development/is-robot/items/edit/' . $robot['robot_id']) . ' &nbsp; ' . 
               l(t('Delete'), 'admin/config/development/is-robot/items/delete/' . $robot['robot_id']);
    }
    else {
      $row[] = 'N/A';
    }
    $rows[] = $row;
  }
  
  if (count($rows) == 0) {
    $rows = array(
      array(
        'data' => array(array('align' => 'center', 'colspan' => 4, 'data' => t('THERE ARE CURRENTLY NO EXTRA USER AGENTS CONFIGURED')))
      ),
    );
  }
  $output = theme('table', array('header'=>$header, 'rows' => $rows));
  return $output;
}

/**
 * A form for creating custom robot entries
 *
 * These are grouped in with module hooks
 */
function is_robot_additional_item_form($form, &$form_status, $prepopulate = NULL) {
  if (is_object($prepopulate)) {
    $prepopulate = array(
      'is_robot_id' => $prepopulate->robot_id,
      'is_robot_user_agent' => $prepopulate->user_agent,
    );
  }
  $form['is_robot_original_id'] = array(
    '#type' => 'hidden',
    '#default_value' => !empty($prepopulate['is_robot_id']) ? $prepopulate['is_robot_id'] : NULL,
  );
  $form['is_robot_id'] = array(
    '#title' => t('Robot ID'),
    '#type' => 'textfield',
    '#maxlength' => 64,
    '#default_value' => !empty($prepopulate['is_robot_id']) ? $prepopulate['is_robot_id'] : NULL,
    '#required' => TRUE,
  );
  $form['is_robot_user_agent'] = array(
    '#title' => t('Robot User Agent'),
    '#type' => 'textarea',
    '#default_value' => !empty($prepopulate['is_robot_user_agent']) ? $prepopulate['is_robot_user_agent'] : NULL,
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  return $form;
}

/**
 * Callback to process additional item
 */
function is_robot_additional_item_form_submit($form, &$form_state) {
  if (!empty($form_state['values']['is_robot_original_id'])) {
    if ($form_state['values']['is_robot_original_id'] == $form_state['values']['is_robot_id']) {
      $write = array(
        'robot_id' => $form_state['values']['is_robot_id'],
        'user_agent' => $form_state['values']['is_robot_user_agent'],
        'is_manual' => 1,
        'data' => serialize($form_state['values']),
      );
      drupal_write_record('is_robot', $write, 'robot_id');
      drupal_goto('admin/config/development/is-robot/items');
    }
    else {
      // replacing, so remove the old
      db_query('DELETE FROM {is_robot} WHERE robot_id = :robot_id', 
        array(':robot_id' => $form_state['values']['is_robot_original_id']));
    }
  }
    
  $write = array(
    'robot_id' => $form_state['values']['is_robot_id'],
    'user_agent' => $form_state['values']['is_robot_user_agent'],
    'is_manual' => 1,
    'data' => serialize($form_state['values']),
  );
  drupal_write_record('is_robot', $write);
  drupal_goto('admin/config/development/is-robot/items');
}

/**
 * Our confirmation page to delete an item from the additional user_agents
 */
function is_robot_delete_confirm($form, &$form_state) {
  $robot = $form_state['build_info']['args'][0];
  $form['robot_id'] = array('#type' => 'hidden', '#value' => $robot->robot_id);
  $form['user_agent'] = array('#type' => 'hidden', '#value' => $robot->user_agent);
  return confirm_form( $form,
                       t('Are you sure you wish to delete the user agent %user_agent?', array('%user_agent' => $robot->user_agent)),
                       'admin/config/development/is-robot/items',
                       t('This action cannot be undone.'),
                       t('Delete'),
                       t('Cancel')
                     );
}

/**
 * callback to do the deletion
 */
function is_robot_delete_confirm_submit($form, &$form_state) {
  $robot_id = $form_state['values']['robot_id'];
  db_query("DELETE FROM {is_robot} WHERE robot_id=:robot_id", array(':robot_id' => $robot_id));
  drupal_set_message(t('%list successfully deleted', array('%list' => $form_state['values']['user_agent'])));
  drupal_goto('admin/config/development/is-robot/items');
}
