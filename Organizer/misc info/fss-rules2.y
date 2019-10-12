
file: header entries;

header: normal_data '\n' simp_attrib entry;

entries: entry | entry entries;

entry: statement '\n' entry_header_line '\n' '\n' attributes;

attributes: attribute | attribute attributes;

attribute: simp_attrib | multline_attrib | vbtm_attrib;

simp_attrib: statement ':' data '\n';

multiline_attrib: statement ':' data '\n' mla_lines;
mla_lines: mla_line | mla_line mla_lines;
mla_line: ' ' normal_data '\n';

vbtm_attrib = statement ':' '\n' lines;
lines = normal_data '\n' | normal_data '\n' lines;
