#include "attribute.h"

void attribute_usage() {
    fprintf(stderr, "\tattribute <attribute-name>\n");
}

static int list_attribute(policydb_t * policydb, char *name)
{
    struct type_datum *attr;
    struct ebitmap_node *n;
    unsigned int bit;

    attr = hashtab_search(policydb->p_types.table, name);
    if (!attr) {
        fprintf(stderr, "%s is not defined in this policy.\n", name);
        return -1;
    }

    if (attr->flavor != TYPE_ATTRIB) {
        fprintf(stderr, "%s is a type not an attribute in this policy.\n", name);
        return -1;
    }

    ebitmap_for_each_bit(&policydb->attr_type_map[attr->s.value - 1], n, bit) {
        if (!ebitmap_node_get_bit(n, bit))
            continue;
        printf("%s\n", policydb->p_type_val_to_name[bit]);
    }

    return 0;
}

int attribute_func (int argc, char **argv, policydb_t *policydb) {
    if (argc != 2) {
        USAGE_ERROR = true;
        return -1;
    }
    return list_attribute(policydb, argv[1]);
}
