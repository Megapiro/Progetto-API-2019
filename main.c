#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_NAME_LENGTH 35
#define MAP_SIZE 65536

#define NAME_IDENTIFIER '"'
#define BLANK ' '
#define SEMICOLON ';'
#define NONE "none"

/*******************************
           DATA TYPES
 *******************************/

enum commands {
    ADDENT = 0, DELENT = 1, ADDREL = 2, DELREL = 3, REPORT = 4, END = 5
};

typedef enum { false, true } boolean;

typedef enum { RED, BLACK } color;

// represents an entity to be monitored
typedef struct en {
    char entity_name[MAX_NAME_LENGTH];
    struct en* prev;
    struct en* next;
}entity;

// node of receivers
typedef struct n {
    entity* entity;        // entity is the key
    int count;
    struct rbl_tree* involved_tree;
    struct n* father;
    struct n* left;
    struct n* right;
    color color;
}node;

// node of senders and for report
typedef struct nl {
    entity* involved_entity;
    struct nl* father;
    struct nl* left;
    struct nl* right;
    color color;
}little_node;

// RED-BLACK tree of receivers
typedef struct rbb_tree {
    struct n* root;
}rb_big_relations_tree;

// RED-BLACK tree of senders
typedef struct rbl_tree {
    struct nl* root;
}rb_little_relations_tree;

// represents a relation to be monitored, each has two trees, one for receivers and one for report, max count is used for report
typedef struct rel {
    char relation_name[MAX_NAME_LENGTH];
    int max_count;
    rb_little_relations_tree* report_tree;
    rb_big_relations_tree* receiving_entities;
    struct rel* next;
    struct rel* prev;
}relation;

/*****************************************
    DATA STRUCTURES && GLOBAL VARIABLES
 *****************************************/

// hash map containing all monitored entities
entity* entity_map[MAP_SIZE];

// list of relations monitored
struct {
    relation* relations;
    int size;
}relation_list;

// parsing variables
char command[MAX_COMMAND_LENGTH];
char* command_type;
char* first_entity_name;
char* second_entity_name;
char* relation_name;

// nil nodes for RED-BLACk trees
static node* nilb;
static little_node* nill;

// global utility variables
boolean update_max;
int temp_max;
int old_count;
char* temp_name;
node* delent_node;
rb_little_relations_tree* temp_report_tree;

/***************************************
        RED-BLACK TREES FUNCTIONS
 ***************************************/

void build_big_nil_node() {
    nilb = malloc(sizeof(node));
    nilb -> entity = NULL;
    nilb -> count = -1;
    nilb -> involved_tree = NULL;
    nilb -> father = nilb;
    nilb -> left = nilb;
    nilb -> right = nilb;
    nilb -> color = BLACK;
}

void build_little_nil_node() {
    nill = malloc(sizeof(little_node));
    nill -> involved_entity = NULL;
    nill -> father = nill;
    nill -> left = nill;
    nill -> right = nill;
    nill -> color = BLACK;
}

little_node* little_bst(rb_little_relations_tree* rb_little_tree, char* searching_name) {
    little_node* node_helper = rb_little_tree -> root;

    while (node_helper != nill) {
        if(strcmp(searching_name, node_helper -> involved_entity -> entity_name) == 0) {
            return node_helper;
        } else if (strcmp(searching_name, node_helper -> involved_entity -> entity_name) < 0) {
            node_helper = node_helper -> left;
        } else {
            node_helper = node_helper -> right;
        }
    }

    return NULL;
}

little_node* little_position_finder(rb_little_relations_tree* rb_little_tree, char* positioning_name) {
    little_node* node_helper = rb_little_tree -> root;
    little_node* father_helper = nill;

    while (node_helper != nill) {
        father_helper = node_helper;

        if(strcmp(positioning_name, node_helper -> involved_entity -> entity_name) == 0) {
            return NULL;
        } else if (strcmp(positioning_name, node_helper -> involved_entity -> entity_name) < 0) {
            node_helper = node_helper -> left;
        } else {
            node_helper = node_helper -> right;
        }
    }

    return father_helper;
}

void big_left_rotate(rb_big_relations_tree* relation_tree, node* left_node) {
    node* node_helper;

    // left subTree of node_helper becomes right subTree of left_node
    node_helper = left_node -> right;
    left_node -> right = node_helper -> left;

    // if it is not a leaf
    if(node_helper -> left != nilb) {
        node_helper -> left -> father = left_node;
    }

    // father fixup
    node_helper -> father = left_node -> father;
    if(left_node -> father == nilb) {        // if left_node is a root, node helper becomes the root
        relation_tree -> root = node_helper;
    } else if(left_node == left_node -> father -> left) {
        left_node -> father -> left = node_helper;
    } else {
        left_node -> father -> right = node_helper;
    }

    // in the end I put left_node on the left of node_helper
    node_helper -> left = left_node;
    left_node -> father = node_helper;
}

void little_left_rotate(rb_little_relations_tree* relation_tree, little_node* left_node) {
    little_node* node_helper;

    // left subTree of node_helper becomes right subTree of left_node
    node_helper = left_node -> right;
    left_node -> right = node_helper -> left;

    // if it is not a leaf
    if(node_helper -> left != nill) {
        node_helper -> left -> father = left_node;
    }

    // father fixup
    node_helper -> father = left_node -> father;
    if(left_node -> father == nill) {        // if left_node is a root, node helper becomes the root
        relation_tree -> root = node_helper;
    } else if(left_node == left_node -> father -> left) {
        left_node -> father -> left = node_helper;
    } else {
        left_node -> father -> right = node_helper;
    }

    // in the end I put left_node on the left of node_helper
    node_helper -> left = left_node;
    left_node -> father = node_helper;
}

void big_right_rotate(rb_big_relations_tree *relation_tree, node* right_node) {
    node* node_helper;

    // right subTree of node_helper becomes left subTree of right_node
    node_helper = right_node -> left;
    right_node -> left = node_helper -> right;

    // if it is not a leaf
    if(node_helper -> right != nilb) {
        node_helper -> right -> father = right_node;
    }

    // father fixup
    node_helper -> father = right_node -> father;
    if(right_node -> father == nilb) {
        relation_tree -> root = node_helper;
    } else if(right_node == right_node -> father -> left) {
        right_node -> father -> left = node_helper;
    } else {
        right_node -> father -> right = node_helper;
    }

    // in the end I put right_node on the right of node_helper
    node_helper -> right = right_node;
    right_node -> father = node_helper;
}

void little_right_rotate(rb_little_relations_tree* relation_tree, little_node* right_node) {
    little_node* node_helper;

    // right subTree of node_helper becomes left subTree of right_node
    node_helper = right_node -> left;
    right_node -> left = node_helper -> right;

    // if it is not a leaf
    if(node_helper -> right != nill) {
        node_helper -> right -> father = right_node;
    }

    // father fixup
    node_helper -> father = right_node -> father;
    if(right_node -> father == nill) {
        relation_tree -> root = node_helper;
    } else if(right_node == right_node -> father -> left) {
        right_node -> father -> left = node_helper;
    } else {
        right_node -> father -> right = node_helper;
    }

    // in the end I put right_node on the right of node_helper
    node_helper -> right = right_node;
    right_node -> father = node_helper;
}

node* big_tree_minimum(node* starting_node) {
    while(starting_node -> left != nilb) {
        starting_node = starting_node -> left;
    }

    return starting_node;
}

little_node* little_tree_minimum(little_node* starting_node) {
    while(starting_node -> left != nill) {
        starting_node = starting_node -> left;
    }

    return starting_node;
}

void little_tree_helper(rb_little_relations_tree* rb_relation_tree, little_node* starting_node, little_node* side_node) {
    if(starting_node -> father == nill) {
        rb_relation_tree -> root = side_node;
    } else if (starting_node == starting_node -> father -> left) {
        starting_node -> father -> left = side_node;
    } else {
        starting_node -> father -> right = side_node;
    }

    side_node -> father = starting_node -> father;
}

void big_tree_helper(rb_big_relations_tree* rb_relation_tree, node* starting_node, node* side_node) {
    if(starting_node -> father == nilb) {
        rb_relation_tree -> root = side_node;
    } else if (starting_node == starting_node -> father -> left) {
        starting_node -> father -> left = side_node;
    } else {
        starting_node -> father -> right = side_node;
    }

    side_node -> father = starting_node -> father;
}

void rb_big_tree_INSERT_FIXUP(rb_big_relations_tree *rb_relation_tree, node* new_relation) {
    if(new_relation == rb_relation_tree -> root) {      // the node is the first one added, roots are always BLACK!
        rb_relation_tree -> root -> color = BLACK;
    } else {
        node* node_helper;
        node* father_helper = new_relation -> father;

        if(father_helper -> color == RED) {
            if(father_helper == father_helper -> father -> left) {
                node_helper = father_helper -> father -> right;

                if(node_helper -> color == RED) {
                    father_helper -> color = BLACK;
                    node_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    rb_big_tree_INSERT_FIXUP(rb_relation_tree, father_helper -> father);
                } else {
                    if(new_relation == father_helper -> right) {
                        new_relation = father_helper;
                        big_left_rotate(rb_relation_tree, new_relation);
                        father_helper = new_relation -> father;
                    }

                    father_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    big_right_rotate(rb_relation_tree, father_helper -> father);
                }
            } else {
                node_helper = father_helper -> father -> left;

                if(node_helper -> color == RED) {
                    father_helper -> color = BLACK;
                    node_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    rb_big_tree_INSERT_FIXUP(rb_relation_tree, father_helper -> father);
                } else {
                    if(new_relation == father_helper -> left) {
                        new_relation = father_helper;
                        big_right_rotate(rb_relation_tree, new_relation);
                        father_helper = new_relation -> father;
                    }

                    father_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    big_left_rotate(rb_relation_tree, father_helper -> father);
                }
            }
        }
    }
}

void rb_little_tree_INSERT_FIXUP(rb_little_relations_tree* rb_relation_tree, little_node* new_relation) {
    if(new_relation == rb_relation_tree -> root) {      // the node is the first one added, roots are always BLACK!
        rb_relation_tree -> root -> color = BLACK;
    } else {
        little_node* node_helper;
        little_node* father_helper = new_relation -> father;

        if(father_helper -> color == RED) {
            if(father_helper == father_helper -> father -> left) {
                node_helper = father_helper -> father -> right;

                if(node_helper -> color == RED) {
                    father_helper -> color = BLACK;
                    node_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    rb_little_tree_INSERT_FIXUP(rb_relation_tree, father_helper -> father);
                } else {
                    if(new_relation == father_helper -> right) {
                        new_relation = father_helper;
                        little_left_rotate(rb_relation_tree, new_relation);
                        father_helper = new_relation -> father;
                    }

                    father_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    little_right_rotate(rb_relation_tree, father_helper -> father);
                }
            } else {
                node_helper = father_helper -> father -> left;

                if(node_helper -> color == RED) {
                    father_helper -> color = BLACK;
                    node_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    rb_little_tree_INSERT_FIXUP(rb_relation_tree, father_helper -> father);
                } else {
                    if(new_relation == father_helper -> left) {
                        new_relation = father_helper;
                        little_right_rotate(rb_relation_tree, new_relation);
                        father_helper = new_relation -> father;
                    }

                    father_helper -> color = BLACK;
                    father_helper -> father -> color = RED;
                    little_left_rotate(rb_relation_tree, father_helper -> father);
                }
            }
        }
    }
}

void rb_little_tree_INSERT(rb_little_relations_tree* rb_relation_tree, little_node* father_helper, entity* involved_entity) {
    little_node* new_little_node;

    // if here is reached, means that the little node does not exist and then it has to be added
    new_little_node = malloc(sizeof(little_node));
    new_little_node -> involved_entity = involved_entity;
    new_little_node -> father = father_helper;
    new_little_node -> left = nill;
    new_little_node -> right = nill;
    new_little_node -> color = RED;

    // now I can handle the father of the inserting node
    if(father_helper == nill) {
        rb_relation_tree -> root = new_little_node;
    } else if(strcmp(new_little_node -> involved_entity -> entity_name, father_helper -> involved_entity -> entity_name) < 0) {
        father_helper -> left = new_little_node;
    } else {
        father_helper -> right = new_little_node;
    }

    // now that I have already added the leaves and set the RED color to the new_relation I fix the balance of the rb tree
    rb_little_tree_INSERT_FIXUP(rb_relation_tree, new_little_node);
}

void rb_big_tree_INSERT(rb_big_relations_tree *rb_relation_tree, entity* new_entity, entity* involved_entity) {
    node* new_big_node;
    node* father_helper = nilb;
    node* node_helper = rb_relation_tree -> root;
    little_node* little_adding_pos = nill;

    // first I find the correct position
    while(node_helper != nilb) {
        father_helper = node_helper;

        if(strcmp(new_entity -> entity_name, node_helper -> entity -> entity_name) == 0) { // checking node is present, I temporally add it
            ++node_helper -> count;
            little_adding_pos = little_position_finder(node_helper -> involved_tree, involved_entity -> entity_name);

            if(little_adding_pos == NULL) {
                --node_helper -> count;
                temp_max = -1;
                return;
            } else {
                rb_little_tree_INSERT(node_helper -> involved_tree, little_adding_pos, involved_entity);
                temp_max = node_helper -> count;
                return;
            }
        } else if (strcmp(new_entity -> entity_name, node_helper -> entity -> entity_name) < 0) {
            node_helper = node_helper -> left;
        } else {
            node_helper = node_helper -> right;
        }
    }

    // if here is reached, means that the starting node does not exist and then the relation neither for sure
    new_big_node = malloc(sizeof(node));
    new_big_node -> involved_tree = malloc(sizeof(rb_little_relations_tree));
    new_big_node -> involved_tree -> root = nill;
    new_big_node -> entity = new_entity;
    new_big_node -> count = 1;
    new_big_node -> father = father_helper;
    new_big_node -> left = nilb;
    new_big_node -> right = nilb;
    new_big_node -> color = RED;

    // now I can handle the father of the inserting node and the little one
    if(father_helper == nilb) {
        rb_relation_tree -> root = new_big_node;
    } else if(strcmp(new_big_node -> entity -> entity_name, father_helper -> entity -> entity_name) < 0) {
        father_helper -> left = new_big_node;
    } else {
        father_helper -> right = new_big_node;
    }

    // now that I have already added the leaves and set the RED color to the new_relation I fix the balance of the rb tree
    temp_max = new_big_node -> count;
    rb_big_tree_INSERT_FIXUP(rb_relation_tree, new_big_node);
    rb_little_tree_INSERT(new_big_node -> involved_tree, little_adding_pos, involved_entity);
}

void rb_relations_DELETE_FIXUP(rb_big_relations_tree* rb_relation_tree, node* deleting_node) {
    node* node_helper;

    while(deleting_node != rb_relation_tree -> root && deleting_node -> color == BLACK) {
        if(deleting_node == deleting_node -> father -> left) {
            node_helper = deleting_node -> father -> right;

            if(node_helper -> color == RED) {
                node_helper -> color = BLACK;
                deleting_node -> father -> color = RED;
                big_left_rotate(rb_relation_tree, deleting_node -> father);
                node_helper = deleting_node -> father -> right;
            }

            if(node_helper -> left -> color == BLACK && node_helper -> right -> color == BLACK) {
                node_helper -> color = RED;
                deleting_node = deleting_node -> father;
            } else {
                if(node_helper -> right -> color == BLACK) {
                    node_helper -> left -> color = BLACK;
                    node_helper -> color = RED;
                    big_right_rotate(rb_relation_tree, node_helper);
                    node_helper = deleting_node -> father -> right;
                }

                node_helper -> color = deleting_node -> father -> color;
                deleting_node -> father -> color = BLACK;
                node_helper -> right -> color = BLACK;
                big_left_rotate(rb_relation_tree, deleting_node -> father);
                deleting_node = rb_relation_tree -> root;
            }
        } else {
            node_helper = deleting_node -> father -> left;

            if(node_helper -> color == RED) {
                node_helper -> color = BLACK;
                deleting_node -> father -> color = RED;
                big_right_rotate(rb_relation_tree, deleting_node -> father);
                node_helper = deleting_node -> father -> left;
            }

            if(node_helper -> right -> color == BLACK && node_helper -> left -> color == BLACK) {
                node_helper -> color = RED;
                deleting_node = deleting_node -> father;
            } else {
                if(node_helper -> left -> color == BLACK) {
                    node_helper -> right -> color = BLACK;
                    node_helper -> color = RED;
                    big_left_rotate(rb_relation_tree, node_helper);
                    node_helper = deleting_node -> father -> left;
                }

                node_helper -> color = deleting_node -> father -> color;
                deleting_node -> father -> color = BLACK;
                node_helper -> left -> color = BLACK;
                big_right_rotate(rb_relation_tree, deleting_node -> father);
                deleting_node = rb_relation_tree -> root;
            }
        }
    }

    deleting_node -> color = BLACK;
}

void rb_little_tree_DELETE_FIXUP(rb_little_relations_tree* rb_relation_tree, little_node* deleting_node) {
    little_node* node_helper;

    while(deleting_node != rb_relation_tree -> root && deleting_node -> color == BLACK) {
        if(deleting_node == deleting_node -> father -> left) {
            node_helper = deleting_node -> father -> right;

            if(node_helper -> color == RED) {
                node_helper -> color = BLACK;
                deleting_node -> father -> color = RED;
                little_left_rotate(rb_relation_tree, deleting_node -> father);
                node_helper = deleting_node -> father -> right;
            }

            if(node_helper -> left -> color == BLACK && node_helper -> right -> color == BLACK) {
                node_helper -> color = RED;
                deleting_node = deleting_node -> father;
            } else {
                if(node_helper -> right -> color == BLACK) {
                    node_helper -> left -> color = BLACK;
                    node_helper -> color = RED;
                    little_right_rotate(rb_relation_tree, node_helper);
                    node_helper = deleting_node -> father -> right;
                }

                node_helper -> color = deleting_node -> father -> color;
                deleting_node -> father -> color = BLACK;
                node_helper -> right -> color = BLACK;
                little_left_rotate(rb_relation_tree, deleting_node -> father);
                deleting_node = rb_relation_tree -> root;
            }
        } else {
            node_helper = deleting_node -> father -> left;

            if(node_helper -> color == RED) {
                node_helper -> color = BLACK;
                deleting_node -> father -> color = RED;
                little_right_rotate(rb_relation_tree, deleting_node -> father);
                node_helper = deleting_node -> father -> left;
            }

            if(node_helper -> right -> color == BLACK && node_helper -> left -> color == BLACK) {
                node_helper -> color = RED;
                deleting_node = deleting_node -> father;
            } else {
                if(node_helper -> left -> color == BLACK) {
                    node_helper -> right -> color = BLACK;
                    node_helper -> color = RED;
                    little_left_rotate(rb_relation_tree, node_helper);
                    node_helper = deleting_node -> father -> left;
                }

                node_helper -> color = deleting_node -> father -> color;
                deleting_node -> father -> color = BLACK;
                node_helper -> left -> color = BLACK;
                little_right_rotate(rb_relation_tree, deleting_node -> father);
                deleting_node = rb_relation_tree -> root;
            }
        }
    }

    deleting_node -> color = BLACK;
}

void rb_little_tree_DELETE(rb_little_relations_tree* rb_relation_tree, little_node* deleting_node) {
    little_node* node_helper;
    little_node* father_helper = deleting_node;
    color father_old_color = father_helper -> color;

    if(deleting_node -> left == nill) {
        node_helper = deleting_node -> right;
        little_tree_helper(rb_relation_tree, deleting_node, deleting_node -> right);
    } else if(deleting_node -> right == nill) {
        node_helper = deleting_node -> left;
        little_tree_helper(rb_relation_tree, deleting_node, deleting_node -> left);
    } else {
        father_helper = little_tree_minimum(deleting_node -> right);
        father_old_color = father_helper -> color;
        node_helper = father_helper -> right;

        if(father_helper -> father == deleting_node) {
            node_helper -> father = father_helper;
        } else {
            little_tree_helper(rb_relation_tree, father_helper, father_helper -> right);
            father_helper -> right = deleting_node -> right;
            father_helper -> right -> father = father_helper;
        }

        little_tree_helper(rb_relation_tree, deleting_node, father_helper);
        father_helper -> left = deleting_node -> left;
        father_helper -> left -> father = father_helper;
        father_helper -> color = deleting_node -> color;
    }

    if(father_old_color == BLACK) {
        rb_little_tree_DELETE_FIXUP(rb_relation_tree, node_helper);
    }

    free(deleting_node);
}

void rb_relations_DELETE(rb_big_relations_tree* rb_relation_tree, node* deleting_node) {
    node* node_helper;
    node* father_helper = deleting_node;
    color father_old_color = father_helper -> color;

    if(deleting_node -> left == nilb) {
        node_helper = deleting_node -> right;
        big_tree_helper(rb_relation_tree, deleting_node, deleting_node -> right);
    } else if(deleting_node -> right == nilb) {
        node_helper = deleting_node -> left;
        big_tree_helper(rb_relation_tree, deleting_node, deleting_node -> left);
    } else {
        father_helper = big_tree_minimum(deleting_node -> right);
        father_old_color = father_helper -> color;
        node_helper = father_helper -> right;

        if(father_helper -> father == deleting_node) {
            node_helper -> father = father_helper;
        } else {
            big_tree_helper(rb_relation_tree, father_helper, father_helper -> right);
            father_helper -> right = deleting_node -> right;
            father_helper -> right -> father = father_helper;
        }

        big_tree_helper(rb_relation_tree, deleting_node, father_helper);
        father_helper -> left = deleting_node -> left;
        father_helper -> left -> father = father_helper;
        father_helper -> color = deleting_node -> color;
    }

    if(father_old_color == BLACK) {
        rb_relations_DELETE_FIXUP(rb_relation_tree, node_helper);
    }

    free(deleting_node);
}

void free_big_tree(node* moving_node) {
    if(moving_node != nilb) {
        free_big_tree(moving_node -> left);
        free_big_tree(moving_node -> right);

        free(moving_node);
    }
}

void free_little_tree(little_node* moving_node) {
    if(moving_node != nill) {
        free_little_tree(moving_node -> left);
        free_little_tree(moving_node -> right);

        free(moving_node);
    }
}

/*******************************
         UTILITY METHODS
 *******************************/

int parse_command() {
    fgets(command, MAX_COMMAND_LENGTH + 1, stdin);
    command_type = strtok(command, " \"");

    if (strcmp(command_type, "addent") == 0) {
        first_entity_name = strtok(NULL, "\"");
        return ADDENT;
    } else if (strcmp(command_type, "delent") == 0) {
        first_entity_name = strtok(NULL, "\"");
        return DELENT;
    } else if (strcmp(command_type, "addrel") == 0) {
        first_entity_name = strtok(NULL, "\" \"");
        second_entity_name = strtok(NULL, "\" \"");
        relation_name = strtok(NULL, "\" \"");
        return ADDREL;
    } else if (strcmp(command_type, "delrel") == 0) {
        first_entity_name = strtok(NULL, "\" \"");
        second_entity_name = strtok(NULL, "\" \"");
        relation_name = strtok(NULL, "\" \"");
        return DELREL;
    } else if (strcmp(command_type, "report\n") == 0) {
        return REPORT;
    } else {
        return END;
    }
}

unsigned int my_hash_djb2(unsigned char* name) {
    unsigned int hash = 5381;
    int c;

    while ((c = *name++)) {
        hash = ((hash << 5u) + hash) + c;
    }

    return hash % MAP_SIZE;
}

entity* check_entity_presence(char* entity_name) {
    unsigned int key = my_hash_djb2((unsigned char*) entity_name);
    entity* presence_checker = entity_map[key];

    if(presence_checker == NULL) {
        return NULL;
    }

    while(presence_checker != NULL) {
        if(strcmp(entity_name, presence_checker -> entity_name) == 0) {
            return presence_checker;
        }

        presence_checker = presence_checker -> next;
    }

    return NULL;
}

void add_first_relation(relation** adding_relation) {
    relation_list.relations = malloc(sizeof(relation));
    relation_list.relations -> report_tree = malloc(sizeof(rb_little_relations_tree));
    relation_list.relations -> receiving_entities = malloc(sizeof(rb_big_relations_tree));

    // tree initialization
    relation_list.relations -> report_tree -> root = nill;
    relation_list.relations -> receiving_entities -> root = nilb;

    strcpy(relation_list.relations -> relation_name, relation_name);
    relation_list.relations -> max_count = 0;
    relation_list.relations -> next = NULL;
    relation_list.relations -> prev = NULL;

    ++relation_list.size;
    *adding_relation = relation_list.relations;
}

void add_relation_in_pos(relation** adding_relation) {
    relation* new_relation = malloc(sizeof(relation));
    new_relation -> report_tree = malloc(sizeof(rb_little_relations_tree));
    new_relation -> receiving_entities = malloc(sizeof(rb_big_relations_tree));

    // tree initialization
    new_relation -> report_tree -> root = nill;
    new_relation -> receiving_entities -> root = nilb;

    strcpy(new_relation -> relation_name, relation_name);
    new_relation -> max_count = 0;

    if(strcmp((*adding_relation) -> relation_name, relation_name) < 0) {
        new_relation -> next = (*adding_relation) -> next;
        (*adding_relation) -> next = new_relation;
        new_relation -> prev = *adding_relation;
    } else {
        if((*adding_relation) -> prev == NULL) {
            relation_list.relations = new_relation;
        } else {
            (*adding_relation) -> prev -> next = new_relation;
        }
        new_relation -> prev = (*adding_relation) -> prev;
        new_relation -> next = *adding_relation;
        (*adding_relation) -> prev = new_relation;
    }

    // in the end I assign the real pointer to the added relation
    ++relation_list.size;
    *adding_relation = new_relation;
}

void update_report_tree(node* moving_node) {
    // instead, with an inorder walk I search for the new max
    if(moving_node != nilb) {
        update_report_tree(moving_node -> left);

        if(moving_node -> count == temp_max) {
            rb_little_tree_INSERT(temp_report_tree, little_position_finder(temp_report_tree, moving_node -> entity -> entity_name), moving_node -> entity);
        } else if(moving_node -> count > temp_max) {
            temp_max = moving_node -> count;
            free_little_tree(temp_report_tree -> root);
            temp_report_tree -> root = nill;
            rb_little_tree_INSERT(temp_report_tree, little_position_finder(temp_report_tree, moving_node -> entity -> entity_name), moving_node -> entity);
        }

        update_report_tree(moving_node -> right);
    }
}

void relations_deleter(node* moving_node) {
    if(moving_node != nilb) {
        relations_deleter(moving_node -> left);
        relations_deleter(moving_node -> right);

        if(strcmp(temp_name, moving_node -> entity -> entity_name) == 0) {
            if(moving_node -> count == temp_max) {
                update_max = true;
            }
            moving_node -> count = 0;
            free_little_tree(moving_node -> involved_tree -> root);
            delent_node = moving_node;
        } else {
            little_node* searching_relation_node = little_bst(moving_node -> involved_tree, temp_name);

            if(searching_relation_node != NULL) {   // means a relation has to be deleted
                if(moving_node -> count == temp_max) {
                    update_max = true;
                }

                --moving_node -> count;
                rb_little_tree_DELETE(moving_node -> involved_tree, searching_relation_node);
            }
        }
    }
}

void rb_relations_search_delrel(relation* deleting_relation, char *entity_name, char *involved_entity) {
    node* big_deleting_node = nilb;
    little_node* little_deleting_node = nill;
    node* node_helper = deleting_relation -> receiving_entities -> root;

    // first I find the position of the deleting node
    while(node_helper != nilb) {
        if(strcmp(entity_name, node_helper -> entity -> entity_name) == 0) { // I have probably found the node of the deleting relation
            little_deleting_node = little_bst(node_helper -> involved_tree, involved_entity);

            if(little_deleting_node != NULL) {
                big_deleting_node = node_helper;
                break;
            } else {
                return;
            }
        } else if (strcmp(entity_name, node_helper -> entity -> entity_name) < 0) {
            node_helper = node_helper -> left;
        } else {
            node_helper = node_helper -> right;
        }
    }

    // first I check that the node is present in the tree
    if(big_deleting_node == nilb) {
        return;
    }

    // now I decrease the node counter and if it reaches 0 occurrences I have to delete it
    --big_deleting_node -> count;
    old_count = big_deleting_node -> count;
    if(big_deleting_node -> count > 0) {
        rb_little_tree_DELETE(big_deleting_node -> involved_tree, little_deleting_node);
    } else {
        rb_little_tree_DELETE(big_deleting_node -> involved_tree, little_deleting_node);
        rb_relations_DELETE(deleting_relation -> receiving_entities, big_deleting_node);
    }

    // in the end I check if the max counter has changed
    if(deleting_relation -> max_count - old_count == 1) {      // in this case I have to find the new maximum occurrences
        // if all relations are being deleted the count is 0
        if(deleting_relation -> receiving_entities -> root == nilb) {
            deleting_relation -> max_count = 0;
        } else if(strcmp(deleting_relation -> report_tree -> root -> involved_entity -> entity_name, entity_name) == 0 &&
                        deleting_relation -> report_tree -> root -> left == nill && deleting_relation -> report_tree -> root -> right) {
            temp_max = 0;
            free_little_tree(deleting_relation -> report_tree -> root);
            deleting_relation -> report_tree -> root = nill;
            temp_report_tree = deleting_relation -> report_tree;
            update_report_tree(deleting_relation -> receiving_entities -> root);
            deleting_relation -> max_count = temp_max;
        } else {
            rb_little_tree_DELETE(deleting_relation -> report_tree, little_bst(deleting_relation -> report_tree, entity_name));
        }
    }
}

void rb_relations_delent_delrel(char* entity_name) {
    relation* relation_pointer = relation_list.relations;

    temp_name = entity_name;
    while (relation_pointer != NULL) {
        // I have to check all the receiving nodes, if the node is not the deleting entity one's then I try to delete
        // a possible relation, otherwise I delete the entire node and all related relations
        temp_max = relation_pointer -> max_count;
        update_max = false;
        delent_node = nilb;
        relations_deleter(relation_pointer -> receiving_entities -> root);

        if(delent_node != nilb) {
            rb_relations_DELETE(relation_pointer -> receiving_entities, delent_node);
        }

        if(update_max) {
            // before moving to the next relation I update the max count of the current relation
            temp_max = 0;
            free_little_tree(relation_pointer -> report_tree -> root);
            relation_pointer -> report_tree -> root = nill;
            temp_report_tree = relation_pointer -> report_tree;
            update_report_tree(relation_pointer -> receiving_entities -> root);
            relation_pointer -> max_count = temp_max;
        }

        // in the end I check if no more relations exist of the kind deleted
        if(relation_pointer -> max_count == 0) {
            if(relation_pointer -> prev == NULL) {
                relation_list.relations = relation_pointer -> next;
            } else {
                relation_pointer -> prev -> next = relation_pointer -> next;
            }

            if(relation_pointer -> next != NULL) {
                relation_pointer -> next -> prev = relation_pointer -> prev;
            }

            free_little_tree(relation_pointer -> report_tree -> root);
            free_big_tree(relation_pointer -> receiving_entities -> root);
            free(relation_pointer);
            --relation_list.size;
        }

        relation_pointer = relation_pointer -> next;
    }
}

void inorder_report_printer(little_node* moving_node) {
    if(moving_node != nill) {
        inorder_report_printer(moving_node -> left);

        putchar(BLANK);
        putchar(NAME_IDENTIFIER);
        fputs(moving_node -> involved_entity -> entity_name, stdout);
        putchar(NAME_IDENTIFIER);

        inorder_report_printer(moving_node -> right);
    }
}

/*******************************
        COMMANDS HANDLERS
 *******************************/
void handle_ADDENT() {
    unsigned int key = my_hash_djb2((unsigned char*) first_entity_name);

    /* the hash map resolves collisions with concatenation, then:
     *          (i) No value for key -> normal add
     *          (ii) Value already present -> new entity is appended in first position if not already present
     */
    if(entity_map[key] == NULL) {
        entity_map[key] = malloc(sizeof(entity));
        strcpy(entity_map[key] -> entity_name, first_entity_name);
        entity_map[key] -> prev = NULL;
        entity_map[key] -> next = NULL;
    } else {
        boolean present = false;
        entity* presence_checker = entity_map[key];

        while(presence_checker != NULL) {
            if (strcmp(first_entity_name, presence_checker -> entity_name) == 0) {
                present = true;
                break;
            }

            presence_checker = presence_checker->next;
        }

        // then, if not present, I add the entity
        if(!present) {
            entity* new_entity = malloc(sizeof(entity));
            strcpy(entity_map[key] -> entity_name, first_entity_name);
            new_entity -> prev = NULL;
            new_entity -> next = entity_map[key];
            new_entity -> next -> prev = new_entity;
            entity_map[key] = new_entity;
        }
    }
}

void handle_DELENT() {
    unsigned int key = my_hash_djb2((unsigned char*) first_entity_name);

    if(entity_map[key] == NULL) {
        return;
    } else {
        entity* presence_checker = entity_map[key];

        // first I check the presence in the concatenated list
        if(strcmp(first_entity_name, presence_checker -> entity_name) == 0) {
            entity* deleting_entity = presence_checker;
            entity_map[key] = presence_checker -> next;
            if(entity_map[key] != NULL) {
                entity_map[key] -> prev = NULL;
            }

            rb_relations_delent_delrel(first_entity_name);
            free(deleting_entity);
            return;
        }

        while(presence_checker -> next -> next != NULL) {
            if(strcmp(first_entity_name, presence_checker -> entity_name) == 0) {
                entity* deleting_entity = presence_checker;
                presence_checker -> prev -> next = presence_checker -> next;
                presence_checker -> next -> prev = presence_checker -> prev;

                rb_relations_delent_delrel(first_entity_name);
                free(deleting_entity);
                return;
            }

            presence_checker = presence_checker -> next;
        }

        presence_checker = presence_checker -> next;
        if(strcmp(first_entity_name, presence_checker -> entity_name) == 0) {
            entity* deleting_entity = presence_checker;
            presence_checker -> prev -> next = NULL;

            rb_relations_delent_delrel(first_entity_name);
            free(deleting_entity);
        }
    }
}

void handle_ADDREL() {
    entity* starting_entity;
    entity* receiving_entity;
    relation* adding_relation;
    little_node* little_node_pos;

    // first I check if both entities are present
    starting_entity = check_entity_presence(first_entity_name);
    receiving_entity = check_entity_presence(second_entity_name);
    if(starting_entity == NULL || receiving_entity == NULL) {
        return;
    }

    adding_relation = relation_list.relations;
    if(relation_list.size == 0) {
        add_first_relation(&adding_relation);
    } else {
        boolean present = false;
        relation* presence_checker = relation_list.relations;

        while(presence_checker != NULL) {
            if(strcmp(presence_checker -> relation_name, relation_name) == 0) {
                adding_relation = presence_checker;
                present = true;
                break;
            } else if(strcmp(presence_checker -> relation_name, relation_name) > 0) {
                adding_relation = presence_checker;
                break;
            }

            adding_relation = presence_checker;
            presence_checker = presence_checker -> next;
        }

        // if the relation is not present in the main list I add it
        if(!present) {
            add_relation_in_pos(&adding_relation);
        }
    }

    // now I INSERT the receiving node in the relative tree
    temp_max = 0;
    rb_big_tree_INSERT(adding_relation -> receiving_entities, receiving_entity, starting_entity);

    if(temp_max == adding_relation -> max_count) {
        little_node_pos = little_position_finder(adding_relation -> report_tree, receiving_entity -> entity_name);
        if(little_node_pos != NULL) {
            rb_little_tree_INSERT(adding_relation -> report_tree, little_node_pos, receiving_entity);
        }
    } else if(temp_max > adding_relation -> max_count) {
        adding_relation -> max_count = temp_max;
        free_little_tree(adding_relation -> report_tree -> root);
        adding_relation -> report_tree -> root = nill;
        rb_little_tree_INSERT(adding_relation -> report_tree, little_position_finder(adding_relation -> report_tree, receiving_entity -> entity_name), receiving_entity);
    }
}

void handle_DELREL() {
    entity* starting_entity;
    entity* receiving_entity;
    relation* deleting_relation;

    // first I check if both entities are present
    starting_entity = check_entity_presence(first_entity_name);
    receiving_entity = check_entity_presence(second_entity_name);
    if(starting_entity == NULL || receiving_entity == NULL) {
        return;
    }

    // then if the relation is present
    deleting_relation = relation_list.relations;
    if(relation_list.size == 0) {
        return;
    } else {
        boolean present = false;
        relation* presence_checker = relation_list.relations;

        while(presence_checker != NULL) {
            if(strcmp(relation_name, presence_checker -> relation_name) == 0) {
                deleting_relation = presence_checker;
                present = true;
                break;
            }

            deleting_relation = presence_checker;
            presence_checker = presence_checker -> next;
        }

        if(!present) {
            return;
        }
    }

    rb_relations_search_delrel(deleting_relation, second_entity_name, first_entity_name);

    // in the end I check if no more relations exist of the kind deleted
    if(deleting_relation -> max_count == 0) {
        if(deleting_relation -> prev == NULL) {
            relation_list.relations = deleting_relation -> next;
        } else {
            deleting_relation -> prev -> next = deleting_relation -> next;
        }

        if(deleting_relation -> next != NULL) {
            deleting_relation -> next -> prev = deleting_relation -> prev;
        }

        free_little_tree(deleting_relation -> report_tree -> root);
        free_big_tree(deleting_relation -> receiving_entities -> root);
        free(deleting_relation);
        --relation_list.size;
    }
}

void handle_REPORT() {
    relation* reporting_relation = relation_list.relations;

    if(relation_list.size == 0) {
        puts(NONE);
        return;
    }

    while(reporting_relation != NULL) {
        temp_max = reporting_relation -> max_count;
        putchar(NAME_IDENTIFIER);
        fputs(reporting_relation -> relation_name, stdout);
        putchar(NAME_IDENTIFIER);
        inorder_report_printer(reporting_relation -> report_tree -> root);
        putchar(BLANK);
        printf("%d", temp_max);

        if(reporting_relation -> next == NULL) {
            putchar(SEMICOLON);
        } else {
            putchar(SEMICOLON);
            putchar(BLANK);
        }

        reporting_relation = reporting_relation -> next;
    }

    fputs("\n", stdout);
}

/**
 * first method called whenever a command is parsed
 */
boolean handle_command() {
    int command_chosen = parse_command();

    switch (command_chosen) {
        case 0:
            handle_ADDENT();
            return false;
        case 1:
            handle_DELENT();
            return false;
        case 2:
            handle_ADDREL();
            return false;
        case 3:
            handle_DELREL();
            return false;
        case 4:
            handle_REPORT();
            return false;
        default: // case 5
            return true;
    }
}

/*******************************
              MAIN
 *******************************/
int main() {
    boolean stop;
    temp_max = 0;
    relation_list.size = 0;
    build_big_nil_node();
    build_little_nil_node();

    do {
        stop = handle_command();
    } while (!stop);

    return 0;
}