<?php

/**
 * @file
 * Contains the forms and logic that are necessary for forum migrate module.
 */

/**
 * This form shows forum taxonomies which will be removed.
 */
function forum_migrate_form($form, &$form_state) {
  $forum_category = taxonomy_vocabulary_machine_name_load('forums');
  $tree = taxonomy_get_tree($forum_category->vid, 0, NULL, TRUE);

  $terms = array();
  $locked_parents = array();

  foreach ($tree as $term) {
    if ($term->parents[0] == 0) {
      $terms[$term->tid] = '<strong>' . t('@term_name (Container)', array('@term_name' => $term->name)) . '</strong>';
    }
    else {
      $parent = taxonomy_term_load($term->parents[0]);
      $terms[$term->tid] = $term->name . ' '.  t('(@parent_term children)', array('@parent_term' => $parent->name));
    }
  }

  $form['categories'] = array(
    '#title' => t('Forum categories'),
    '#type' => 'checkboxes',
    '#required' => TRUE,
    '#options' => $terms,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Remove category and associated content'),
    '#attributes' => array('class' => array('btn-danger')),
    '#weight' => 6,
  );

  return $form;
}

/**
 * Submit callback for the forum migrate form.
 */
function forum_migrate_form_submit($form, &$form_state) {
  $_SESSION['forum_categories'] = $form_state['values']['categories'];

  $form_state['redirect'] = url('forum_migrate/remove/confirm');
}

/**
 * Validates if container to be deleted it's not parent from other category.
 */
function forum_migrate_form_validate($form, &$form_state) {
  $categories = array_diff($form_state['values']['categories'], array(0));

  foreach ($categories as $key => $tid) {
    $children = taxonomy_get_children($tid);
    if (!empty($children)) {
      form_set_error('categories', t('You can\'t delete full container as long as he is parent from another category.'));
    }
  }
}

/**
 * Shows confirmation form to remove forum categories and associated content.
 */
function forum_migrate_remove_confirm_form($form, &$form_state) {
  $_SESSION['forum_categories'] = array_diff($_SESSION['forum_categories'], array(0));
  $options_selected = '<ul>';

  foreach ($_SESSION['forum_categories'] as $forum_category) {
    $category = taxonomy_term_load($forum_category);
      $options_selected .= '<li>' . $category->name . '</li>';
   }
   $options_selected .= '</ul>';

  return confirm_form($form, t('Delete all nodes and comments from the categories selected'),
    'forum_migrate/remove',
    t('Are you sure you want to delete all these items and the associated content? All comments and nodes associated to the categories selected will be removed from the system.') . $options_selected,
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Submit callback for the forum taxonomies list form.
 */
function forum_migrate_remove_confirm_form_submit($form, &$form_state) {
  foreach ($_SESSION['forum_categories'] as $tid => $forum_category) {
    $nids = forum_migrate_get_topics_associated_nodes($forum_category);

    foreach ($nids as $nid) {
      node_delete($nid);
      forum_migrate_remove_forum_node_references($nid);
    }
    taxonomy_term_delete($forum_category);
  }

  $form_state['redirect'] = url('forum_migrate/remove');
}

/**
 * Deletes references that appear in forum tables from nodes being deleted.
 */
function forum_migrate_remove_forum_node_references($nid) {
  db_delete('forum')
    ->condition('nid', $nid)
    ->execute();
  db_delete('forum_index')
    ->condition('nid', $nid)
    ->execute();
  db_delete('forum_access')
    ->condition('tid', $nid)
    ->execute();
}

/**
 * Gets all nodes associated to a forum category.
 */
function forum_migrate_get_topics_associated_nodes($forum_category) {
  $nids = db_select('forum', 'f')
    ->fields('f', array('nid'))
    ->condition('tid', $forum_category)
    ->execute()
    ->fetchAllKeyed();
  $nids = array_keys($nids);

  return $nids;
}

/**
 * Gets all comments associated to a forum node nid.
 */
function forum_migrate_get_associated_node_comments($nid) {
  $cids = db_select('comment', 'c')
    ->fields('c', array('cid'))
    ->condition('nid', $nid)
    ->execute()
    ->fetchAllKeyed();

  $cids = array_keys($nid);

  return $cids;
}
