#!/usr/bin/perl -w
#==============================================================================
#
#    Microsoft Visual C++ project and workspace file generator.
#    Copyright (C) 2000-2004 by Eric Sunshine <sunshine@sunshineco.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
#==============================================================================
#------------------------------------------------------------------------------
# msvcgen.pl
#
#    A tool for generating Microsoft Visual C++ project files and workspaces
#    from a set of templates.  Invoke the --help option for detailed
#    instructions.
#
#------------------------------------------------------------------------------
use strict;
use Digest::MD5 qw(md5_hex);
use File::Basename;
use Getopt::Long;
$Getopt::Long::ignorecase = 0;

my $PROG_NAME = 'msvcgen.pl';
my $PROG_VERSION = 10;
my $AUTHOR_NAME = 'Eric Sunshine';
my $AUTHOR_EMAIL = 'sunshine@sunshineco.com';
my $COPYRIGHT = "Copyright (C) 2000-2004 by $AUTHOR_NAME <$AUTHOR_EMAIL>";

$main::opt_project = 0;
$main::opt_workspace = 0;
$main::opt_template = '';
$main::opt_xml_protect = 0;
@main::opt_delaylib = ();
$main::opt_output = '';
$main::opt_fragment = undef;
@main::opt_depend = ();
$main::opt_template_dir = '';
@main::opt_strip_root = ();
@main::opt_accept = ();
@main::opt_reject = ();
@main::opt_response_file = ();
@main::opt_set = ();
$main::opt_verbose = 0;
$main::opt_v = 0;	# Alias for 'verbose'.
$main::opt_quiet = 0;
$main::opt_version = 0;
$main::opt_V = 0;	# Alias for 'version'.
$main::opt_help = 0;

my @script_options = (
    'project',
    'workspace',
    'template=s',
    'xml-protect',
    'delaylib=s@',
    'output=s',
    'fragment:s',
    'depend=s@',
    'template-dir=s',
    'strip-root=s@',
    'accept=s@',
    'reject=s@',
    'response-file=s@',
    'set=s@',
    'verbose!',
    'v!',		# Alias for 'verbose'.
    'quiet!',
    'version',
    'V',		# Alias for 'version'.
    'help'
);

$main::verbosity = 0;
$main::accept_patterns = '';
$main::reject_patterns = '';
$main::variables = {};
$main::guid = '';
$main::groups = {};
@main::pjf_fragments = ();
@main::dpf_fragments = ();
@main::cff_fragments = ();

$main::project_template = '';
$main::project_group_template = '';
$main::project_file_template = '';
$main::project_delaylib_template = '';

$main::workspace_template = '';
$main::workspace_project_template = '';
$main::workspace_depend_template = '';
$main::workspace_config_template = '';

$main::reserved_variables = qr/^(?:
    configs     |
    delaylib    |
    delaylibs   |
    depend      |
    depends     |
    depguid     |
    depnum      |
    file        |
    files       |
    group       |
    groups      |
    guid        |
    projects    |
    projfile
)$/x;

$main::patterns = {
    'sources'     => {
	'name'    => 'Source Files',
	'pattern' => '(?i)\.(c|cc|cpp|C|m|mm|M)$'
    },
    'headers'     => {
	'name'    => 'Header Files',
	'pattern' => '(?i)\.(h|hpp|H)$'
    },
    'resources'   => {
	'name'    => 'Resource Files'
    }
};

#------------------------------------------------------------------------------
# Emit an error message and terminate abnormally.
#------------------------------------------------------------------------------
sub fatal {
    my $msg = shift;
    die "ERROR: $msg\n";
}

#------------------------------------------------------------------------------
# Emit a warning message.
#------------------------------------------------------------------------------
sub warning {
    my $msg = shift;
    warn "WARNING: $msg\n";
}

#------------------------------------------------------------------------------
# Should we be verbose?
#------------------------------------------------------------------------------
sub verbose {
    return $main::verbosity > 0;
}

#------------------------------------------------------------------------------
# Should we be quiet?
#------------------------------------------------------------------------------
sub quiet {
    return $main::verbosity < 0;
}

#------------------------------------------------------------------------------
# Print an underlined title.
#------------------------------------------------------------------------------
sub print_title {
    my $msg = shift;
    print "$msg\n", '-' x length($msg), "\n";
}

#------------------------------------------------------------------------------
# Add a suffix to a filename if not already present.
#------------------------------------------------------------------------------
sub add_suffix {
    my ($name, $suffix) = @_;
    $name .= ".$suffix" unless $name =~ /(?i)\.$suffix$/;
    return $name;
}

#------------------------------------------------------------------------------
# Remove a suffix from a filename if present.
#------------------------------------------------------------------------------
sub remove_suffix {
    my $name = shift;
    $name =~ s:\.[^/\\.]*$::;
    return $name;
}

#------------------------------------------------------------------------------
# Create a unique GUID from a project name.
#------------------------------------------------------------------------------
sub guid_from_name {
    my $projname = shift;
    my $rawguid = md5_hex($projname);
    my $shapedguid = '{' .
	substr($rawguid,  0,  8) . '-' .
	substr($rawguid,  8,  4) . '-' .
	substr($rawguid, 12,  4) . '-' .
	substr($rawguid, 16,  4) . '-' .
	substr($rawguid, 20, 12) .
	'}';
    return uc($shapedguid);
}

#------------------------------------------------------------------------------
# Protect XML special characters with appropriate XML entity references.
#------------------------------------------------------------------------------
sub xmlize {
    my $result = shift;
    $result =~ s/\"/\&quot\;/g;
    $result =~ s/\</\&lt\;/g;
    $result =~ s/\>/\&gt\;/g;
    return $result;
}

#------------------------------------------------------------------------------
# Protect XML special characters with appropriate XML entity references if the
# --xml-protect option was specified.  If the value is an array reference
# rather than a string, then convert the elements to a single string delimited
# by spaces before applying the XML translation.
#------------------------------------------------------------------------------
sub xmlprotect {
    my $result = shift;
    $result = join(' ',@{$result}) if ref($result) and ref($result) eq 'ARRAY';
    return $main::opt_xml_protect ? xmlize($result) : $result;
}

#------------------------------------------------------------------------------
# Read file contents.
#------------------------------------------------------------------------------
sub load_file {
    my $path = shift;
    my $content = '';
    my $size = -s $path;
    fatal("Failed to load file $path: $!") unless defined($size);
    if ($size) {
	open(FILE, "<$path") or fatal("Unable to open $path: $!");
	binmode(FILE);
	read(FILE, $content, $size) == $size
	    or fatal("Failed to read all $size bytes of $path: $!");
	close(FILE);
    }
    return $content;
}

#------------------------------------------------------------------------------
# Save file contents.
#------------------------------------------------------------------------------
sub save_file {
    my ($path, $content) = @_;
    open (FILE, ">$path") or fatal("Unable to open $path: $!");
    binmode(FILE);
    print FILE $content if length($content);
    close(FILE);
}

#------------------------------------------------------------------------------
# Load a template file.
#------------------------------------------------------------------------------
sub load_template {
    my ($file, $ext) = @_;
    return load_file("$main::opt_template_dir/$file.$ext");
}

#------------------------------------------------------------------------------
# Load templates.
#------------------------------------------------------------------------------
sub load_templates {
    if ($main::opt_workspace) {
	$main::workspace_template = load_template('ws', 'tpl');
    }
    else {
	$main::project_template = load_template("$main::opt_template", 'tpl');
	$main::project_group_template = load_template('prjgroup', 'tpi');
	$main::project_file_template = load_template('prjfile', 'tpi');
	$main::project_delaylib_template = load_template('prjdelay', 'tpi');
	$main::workspace_project_template = load_template('wsgroup', 'tpi');
	$main::workspace_depend_template = load_template('wsdep', 'tpi');
	$main::workspace_config_template = load_template('wscfg', 'tpi');
    }
}

#------------------------------------------------------------------------------
# Interpolate a value into a string.  If the value is an array reference, then
# compose a string out of the array elements and use the result as the
# replacement value.  Otherwise assume that the value is a simple string.
#------------------------------------------------------------------------------
sub interpolate_variable {
    my ($buffer, $pattern, $value) = @_;
    $value = join(' ', @{$value}) if ref($value) and ref($value) eq 'ARRAY';
    ${$buffer} =~ s/$pattern/$value/g;
}

#------------------------------------------------------------------------------
# Interpolate a set of variables into a string.  The set of variables is
# gleaned from the received hash reference where the hash keys are variable
# names, and the hash values are variable values.
#------------------------------------------------------------------------------
sub interpolate_variables {
    my ($buffer, $vars) = @_;
    my $key;
    foreach $key (sort(keys(%{$vars}))) {
	interpolate_variable($buffer, "\%$key\%", $vars->{$key});
    }
}

#------------------------------------------------------------------------------
# Interpolate a set of variables into a string.  The set of variables is
# composed of the global variable list (constructed via --set) and the received
# hash reference where the hash keys are variable names, and the hash values
# are variable values.  Undefined %variable% references in the string are
# removed (as if %variable% expanded to the null string).
#------------------------------------------------------------------------------
sub interpolate {
    my ($buffer, $other_vars) = @_;
    interpolate_variables($buffer, $main::variables);
    interpolate_variables($buffer, $other_vars);
    ${$buffer} =~ s/\%\w+?\%//ig;
}

#------------------------------------------------------------------------------
# For each item in a list, interpolate the item into a template based upon
# a pattern string, and return the concatenation of all resulting buffers.
#------------------------------------------------------------------------------
sub interpolate_items {
    my ($template, $pattern, $items) = @_;
    my $result = '';
    my $item;
    foreach $item (sort(@{$items})) {
	my $buffer = $template;
	interpolate(\$buffer, { $pattern => $item });
	$result .= $buffer;
    }
    return $result;
}

#------------------------------------------------------------------------------
# Build the contents of a single project file group.
#------------------------------------------------------------------------------
sub interpolate_project_group {
    my $group = shift;
    my $files = $main::groups->{$group};
    my $files_buffer =
	interpolate_items($main::project_file_template, 'file', $files);

    my $group_name = $main::patterns->{$group}->{'name'};
    my $result = $main::project_group_template;
    interpolate(\$result, {
	'group' => $group_name,
	'files' => $files_buffer
    });
    return $result;
}

#------------------------------------------------------------------------------
# Build all file-groups for a project from a template.
#------------------------------------------------------------------------------
sub interpolate_project_groups {
    my $result = '';
    my $group;
    foreach $group (sort(keys(%{$main::groups}))) {
        $result .= interpolate_project_group($group);
    }
    return $result;
}

#------------------------------------------------------------------------------
# Build a project from a template.
#------------------------------------------------------------------------------
sub interpolate_project {
    my $result = $main::project_template;
    my $delaylibs = interpolate_items(
	$main::project_delaylib_template, 'delaylib', \@main::opt_delaylib);
    interpolate(\$result, {
	'delaylibs' => xmlprotect($delaylibs),
	'groups'    => interpolate_project_groups()
    });
    return $result;
}

#------------------------------------------------------------------------------
# Build the contents of a workspace dependency group fragment.
#------------------------------------------------------------------------------
sub interpolate_ws_dependency {
    my $result = '';
    my $depcnt = 0;
    my $dependency;
    foreach $dependency (sort(@main::opt_depend)) {
	my $buffer = $main::workspace_depend_template;
	interpolate(\$buffer, {
	    'depnum'  => $depcnt,
	    'guid'    => $main::guid,
	    'depguid' => guid_from_name($dependency),
	    'depend'  => $dependency
	});
	$result .= $buffer;
	$depcnt++;
    }
    return $result;
}

#------------------------------------------------------------------------------
# Build the contents of workspace dependency group fragment.
#------------------------------------------------------------------------------
sub interpolate_ws_project {
    my $depends_buffer = shift;
    my $result = $main::workspace_project_template;
    interpolate(\$result, {
	'projfile' => basename($main::opt_output),
	'depends'  => $depends_buffer,
	'guid'     => $main::guid
    });
    return $result;
}

#------------------------------------------------------------------------------
# Build the contents of workspace configuration group fragment.
#------------------------------------------------------------------------------
sub interpolate_ws_config {
    my $result = $main::workspace_config_template;
    interpolate(\$result, { 'guid' => $main::guid });
    return $result;
}

#------------------------------------------------------------------------------
# Build a workspace from a list of dependency groups fragments.
#------------------------------------------------------------------------------
sub interpolate_workspace {
    my $proj_buffer = '';
    my $depends_buffer = '';
    my $config_buffer = '';
    my $fragment;
    foreach $fragment (sort(@main::pjf_fragments)) {
	$proj_buffer .= load_file($fragment);
    }
    foreach $fragment (sort(@main::dpf_fragments)) {
	$depends_buffer .= load_file($fragment);
    }
    foreach $fragment (sort(@main::cff_fragments)) {
	$config_buffer .= load_file($fragment);
    }
    my $result = $main::workspace_template;
    interpolate(\$result, {
	'projects'   => $proj_buffer,
	'depends'    => $depends_buffer,
	'configs'    => $config_buffer
    });
    return $result;
}

#------------------------------------------------------------------------------
# Create a DSP/VCPROJ file and optionally a DSW/SLN dependency fragment file.
#------------------------------------------------------------------------------
sub create_project {
    save_file($main::opt_output, interpolate_project());
    print "Generated: $main::opt_output\n" unless quiet();

    if (defined($main::opt_fragment)) {
    	my $dependencies = interpolate_ws_dependency();
    	my $dpf_frag = add_suffix($main::opt_fragment, 'dpf');
	save_file($dpf_frag, $dependencies);
	print "Generated: $dpf_frag\n" unless quiet();

    	my $pjf_frag = add_suffix($main::opt_fragment, 'pjf');
	save_file($pjf_frag, interpolate_ws_project($dependencies));
	print "Generated: $pjf_frag\n" unless quiet();

    	my $cff_frag = add_suffix($main::opt_fragment, 'cff');
	save_file($cff_frag, interpolate_ws_config());
	print "Generated: $cff_frag\n" unless quiet();
    }
}

#------------------------------------------------------------------------------
# Create a DSW/SLN workspace file.
#------------------------------------------------------------------------------
sub create_workspace {
    save_file($main::opt_output, interpolate_workspace());
    print "Generated: $main::opt_output\n\n" unless quiet();
}

#------------------------------------------------------------------------------
# Validate project options.
#------------------------------------------------------------------------------
sub validate_project_options {
    usage_error("The --depend option can be used only with --fragment.")
	if !defined($main::opt_fragment) and @main::opt_depend;
    usage_error("Must specify a template type.") unless $main::opt_template;
    usage_error("Must specify --output.") unless $main::opt_output;
}

#------------------------------------------------------------------------------
# Validate workspace options.
#------------------------------------------------------------------------------
sub validate_workspace_options {
    usage_error("The --template option can be used only with --project.")
	if $main::opt_template;
    usage_error("The --fragment option can be used only with --project.")
	if defined($main::opt_fragment);
    usage_error("The --depend option can be used only with --project.")
	if @main::opt_depend;
    usage_error("The --delay-lib option can be used only with --project.")
	if @main::opt_delaylib;
    usage_error("The --strip-root option can be used only with --project.")
	if @main::opt_strip_root;
    usage_error("Must specify --output.") unless $main::opt_output;
}

#------------------------------------------------------------------------------
# Validate options.
#------------------------------------------------------------------------------
sub validate_options {
    usage_error("Must specify --workspace or --project.")
	unless $main::opt_workspace or $main::opt_project;
    usage_error("Must specify --workspace or --project, but not both.")
	if $main::opt_workspace and $main::opt_project;

    validate_workspace_options() if $main::opt_workspace;
    validate_project_options() if $main::opt_project;
}

#------------------------------------------------------------------------------
# Given an array of pattern strings, synthesize a regular expression which
# checks all patterns in parallel.  If the pattern array is empty, the
# $fallback_pattern is returned.
#------------------------------------------------------------------------------
sub synthesize_pattern {
    my ($patterns, $fallback_pattern) = @_;
    my $composite;
    my $pattern;
    foreach $pattern (@{$patterns}) {
	$composite .= '|' if $composite;
	$composite .= $pattern;
    }
    $composite = $fallback_pattern unless $composite;
    return $composite;
}

#------------------------------------------------------------------------------
# Collect interpolation variables created by --set option.
#------------------------------------------------------------------------------
sub collect_variables {
    my $tuple;
    foreach $tuple (@main::opt_set) {
	my ($key, $val) = split(/\s*=\s*/, $tuple);
	$key =~ s/^\s*// if $key;
	next unless $key;
	$val = '' unless $val;
	$val =~ s/\s*$//;
	unless ($key =~ $main::reserved_variables) {
	    $val = xmlprotect($val);
	    if (exists($main::variables->{$key})) {
		$main::variables->{$key} .= " $val";
	    }
	    else {
		$main::variables->{$key} = "$val";
	    }
	}
	else {
	    warning "Reserved variable: \%$key\%; ignoring --set=$key=$val\n";
	}
    }
}

#------------------------------------------------------------------------------
# Interpolate variables created by --set into other variables created by --set.
#------------------------------------------------------------------------------
sub expand_variables {
    my $key;
    foreach $key (keys(%{$main::variables})) {
	my %vars = %{$main::variables};
	my $val = $vars{$key};
	delete $vars{$key};
	interpolate_variables(\$val, \%vars);
	$val =~ s/\%\w+?\%//ig;
	$main::variables->{$key} = $val
	    unless $val eq $main::variables->{$key};
    }
}

#------------------------------------------------------------------------------
# Process options which apply globally (workspace or project mode).
#------------------------------------------------------------------------------
sub process_global_options {
    $main::verbosity =  1 if $main::opt_verbose;
    $main::verbosity = -1 if $main::opt_quiet;
    $main::opt_template_dir = '.' unless $main::opt_template_dir;
    $main::accept_patterns = synthesize_pattern(\@main::opt_accept, '.+');
    $main::reject_patterns = synthesize_pattern(\@main::opt_reject, '^$');

    collect_variables();
    expand_variables();
}

#------------------------------------------------------------------------------
# Process option aliases.
#------------------------------------------------------------------------------
sub process_option_aliases {
    $main::opt_verbose = 1 if $main::opt_v;
    $main::opt_version = 1 if $main::opt_V;
}

#------------------------------------------------------------------------------
# Filter an input list based upon --accept and --reject options.
#------------------------------------------------------------------------------
sub filter {
    return grep(/$main::accept_patterns/i && !/$main::reject_patterns/i, @_);
}

#------------------------------------------------------------------------------
# Massage paths from the command-line by stripping roots specified via
# --strip-root and by translating forward slashes to backward slashes.
#------------------------------------------------------------------------------
sub massage_paths {
    my @infiles = @_;
    my @outfiles;
    my $file;
    foreach $file (@infiles) {
	$file =~ tr:/:\\:;
	my $root;
	foreach $root (@main::opt_strip_root) {
	    last if $file =~ s/^$root//;
	}
	push(@outfiles, $file);
    }
    return @outfiles;
}

#------------------------------------------------------------------------------
# Read a response file and return a list of the contained items.
#------------------------------------------------------------------------------
sub read_response_file {
    my $path = shift;
    my $line;
    my @items;
    open(FILE, "<$path") or fatal("Unable to open response file $path: $!");
    while ($line = <FILE>) {
	$line =~ s/^\s+//;
	$line =~ s/#.*$//;
	$line =~ s/\s+$//;
	next if $line =~ /^$/;
	push(@items, $line);
    }
    close(FILE);
    return @items;
}

#------------------------------------------------------------------------------
# Return a list of input files specified as arguments on the command-line and
# via response files.
#------------------------------------------------------------------------------
sub input_files {
    my @items;
    my $response_file;
    foreach $response_file (@main::opt_response_file) {
	push(@items, read_response_file($response_file));
    }
    push(@items, @ARGV);
    return @items;
}

#------------------------------------------------------------------------------
# Process DSP/VCPROJ project command-line options.
#------------------------------------------------------------------------------
sub process_project_options {
    my $outraw = remove_suffix($main::opt_output);

    $main::opt_fragment = $outraw
	if defined($main::opt_fragment) and !$main::opt_fragment;

    $main::guid = guid_from_name(basename($outraw));

    my %depends;
    my $depend;
    foreach $depend (filter(@main::opt_depend)) {
	$depend = remove_suffix($depend);
	unless (exists($depends{$depend})) {
	    $depends{$depend} = 1;
	}
    }
    @main::opt_depend = keys(%depends);

    my @roots;
    my $root;
    foreach $root (@main::opt_strip_root) {
	$root =~ tr:/:\\:;
	push(@roots, quotemeta($root));
    }
    @main::opt_strip_root = @roots;

    my @files = massage_paths(filter(input_files()));

    my $default_type = '';
    my $type;
    foreach $type (keys(%{$main::patterns})) {
	my $pattern = $main::patterns->{$type}->{'pattern'};
	$default_type = $type, next unless $pattern;
	my @matches = grep(/$pattern/, @files);
	if (@matches) {
	    $main::groups->{$type} = \@matches;
	    @files = grep(!/$pattern/, @files);
	}
    }
    $main::groups->{$default_type} = \@files if @files;
}

#------------------------------------------------------------------------------
# Process DSW/SLN workspace command-line options.
#------------------------------------------------------------------------------
sub process_workspace_options {
    my $fragment;
    foreach $fragment (filter(input_files())) {
	push(@main::pjf_fragments, add_suffix($fragment, 'pjf'));
	push(@main::dpf_fragments, add_suffix($fragment, 'dpf'));
	push(@main::cff_fragments, add_suffix($fragment, 'cff'));
    }
}

#------------------------------------------------------------------------------
# Display an opening banner.
#------------------------------------------------------------------------------
sub banner {
    my $stream = shift;
    $stream = \*STDOUT unless $stream;
    print $stream "\n$PROG_NAME version $PROG_VERSION\n$COPYRIGHT\n\n";
}

#------------------------------------------------------------------------------
# Display usage statement.
#------------------------------------------------------------------------------
sub print_usage {
    my $stream = shift;
    $stream = \*STDOUT unless $stream;
    banner($stream);
    print $stream <<"EOT";
Given a set of input files and project dependencies, generates Microsoft
Visual C++ workspace and project files from a set of templates.

Project files and workspaces can be built for both MSVC 6 and 7, given the
appropriate templates.  The command line options are the same for each version.
The MSVC version for which to generate files is controlled by providing a
suitable set of templates via --template-dir; and by providing the proper file
extension for the project or workspace file named by the --output option.  For
MSVC 6, '.dsp' should be used as the suffix for project files, and '.dsw' for
workspace files.  For MSVC 7, '.vcproj' and '.sln' should be used for project
and workspace files, respectively.  The --xml-protect option should be used
when creating MSVC 7 project files in order to ensure that special characters
(such as ", <, and >) get encoded via XML character references.  This is
needed since MSVC 7 files are stored in XML format.

A project file is generated when --project is specified.  The type of project
represented by the project file is selected using the --template option.  The
template can be one of "appgui", "appcon", "plugin", "library", or "group";
which represent a GUI application, a console application, a dynamic link
library (DLL), a static library (LIB), or a group project, respectively.  The
"group" project type is used for creating pseudo-dependency targets within a
workspace but does not actually generate any resources.

Template files are loaded from the current working directory or from the
directory named via --template-dir.  The template files appgui.tpl, appcon.tpl,
plugin.tpl, library.tpl, and group.tpl correspond to the project types
available via the --template option.  In addition to interpolation variables
created with the --set option, which are available globally, template files may
reference the variables \%groups\%, and %delaylibs%.

The template prjgroup.tpi is used multiple times to build a value for the
\%groups\% variable mentioned above.  This template is used to create the file
groups "Source Files", "Header Files", and "Resource Files", as needed, within
the generated project file.  This template may reference the variables
\%group\% and \%files\%, in addition to the variables available to all
templates.  The \%group\% variable is automatically replaced by the name of the
group being generated (for instance "Source Files").

The template prjfile.tpi is used multiple times to build a value for the
\%files\% variable mentioned above.  This template is used to specify each file
which makes up a file group (represented by prjgroup.tpi).  This template may
reference globally available variables, as well as the variable \%file\% which
represents a file name provided as an argument to this script during DSP/VCPROJ
generation.

The template prjdelay.tpi is used multiple times to bulid a value for the
\%delaylibs\% variable mentioned above.  This template is used to specify each
delayed-load library given via the --delaylib option.  The entire content of
this template should be placed on a single line and should not end with a line
terminator.  This template may reference the variable \%delaylib\%, as well as
the globally available variables.  The value of \%delaylib\% will be a name of
a DLL specified with --delaylib.

During project file generation, a couple of workspace fragment files (for
project, configuration, and dependency information) can also be generated with
the --fragment option and zero or more --depend options.  The generated
fragment files can later be used to create a complete workspace file containing
all projects, configuration information, and dependency graph for the entire
project (to wit, to create a complete, valid workspace referencing all the
project files generated individually).  Note: MSVC 6 projects require only the
dependency information; the templates for the other fragments may be empty.

The project fragment file is created from the template wsgroup.tpi.  This
template may reference the variables \%projfile\%, \%depends\%, and \%guid\%,
as well as the globally available variables.  The \%projfile\% variable is
replaced by the name of the generated project file (see --output), except that
the directory portion of the path is stripped off.  The \%depends\% variable
contains a collection of inter-project dependency information for projects
contained in the workspace.  The value of \%guid\% is a unique identifier which
is required by every MSVC 7 project.  This value is composed automatically from
the output name.

To create configuration fragments, the template wscfg.tpi is used.  It may
reference the variable \%guid\%, in addition to the globally available
variables.  This template is required only by MSVC 7, but it must also be
present when creating MSVC 6 files, though it may left empty.

The template wsdep.tpi is used multiple times to build a value for the
\%depends\% variable mentioned above.  This template is used to specify
inter-project dependencies within a workspace file.  This template may
reference the variables \%guid\%, \%depguid\%, \%depnum\%, \%depend\%, and the
globally available variables.  \%guid\% is the unique project identifier
discussed above.  \%depguid\% is the unique project identifier of a project
upon which this project depends (see --depend) and is useful for MSVC 7
synthesis, whereas \%depend\% is the name of a project upon which this project
depends (see --depend) and is useful for MSVC 6 synthesis.  Each project named
by --depend is also assigned a small number (starting at zero with the first
--depend encountered).  When processing the named dependency, the associated
number is available as \%depnum\%.

Finally, a workspace file is generated when --workspace is specified.  The
DSW/SLN file is created by merging the contents of fragment files into the
template ws.tpl.  This template may reference the variables \%projects\%,
\%depends\% and \%configs\%, which are replaced by the collected contents of
all project, dependency, and configuration fragments named as arguments to this
script for workspace synthesis.

Usage: $PROG_NAME [options] <file> ...

Global Options:
    --project    Generate a project file.
    --workspace  Generate a workspace file.
    --set=<variable>=<value>
                 Create an interpolation variable which can be substituted into
                 template files.  For example, --set=cflags='/D "__DEBUG__"'
                 creates the variable 'cflags' which can be referenced in a
                 template as '\%cflags\%'.  If 'value' is omitted, then
                 'variable' expands to the null string.  Undefined variables
                 also expand to the null string, thus "foo\%notset\%bar"
                 becomes "foobar" if the variable 'notset' is undefined.  This
                 option may be invoked any number of times to create multiple
                 interpolation variables.  If it is invoked multiple times with
                 the same 'variable' name, however, then the individually
                 specified values are collected into a single value, delimited
                 by whitespace.  For instance, "--set=foo=fish --set=foo=stick"
                 gives 'foo' the value "fish stick".  Variable interpolation
                 also occurs within variables created with --set, thus
                 "--set=foo=\%bar\%stick --set=bar=fish" results in 'foo'
                 having the value "fishstick".
    --xml-protect
                 Use XML character references in strings inserted into
                 generated files in place of "special" characters.  Specify
                 this option when creating MSVC 7 project files and workspaces.
    --template-dir=<path>
                 Specifies the directory where the template files reside.  If
                 not specified, then template files are assumed to exist in the
                 current working directory.
    --accept=<pattern>
                 Specifies a Perl regular-expression used as a filter against
                 each named <file>.  Filenames which match the pattern will be
                 included in the generated workspace or project unless
                 overriden by --reject.  The --accept option may be given any
                 number of times in order to specify any number of patterns.
                 This is a useful option for clients unable to filter the list
                 filenames themselves.  Example: --accept='\\.cc\$'
    --reject=<pattern>
                 Specifies a Perl regular-expression used as a filter against
                 each named <file>.  Filenames which match the pattern will not
                 be included in the generated workspace or project.
                 Reject-patterns override accept-patterns.  The --reject option
                 may be given any number of times in order to specify any
                 number of patterns.  This is a useful option for clients
                 unable to filter the list of filenames themselves.
                 Example: --reject='\\.txt\$'
    --response-file=<path>
                 Specifies a file containing pathnames, one per line, which are
                 treated exactly as if they had been mentioned on the
                 command-line as <file>.  The --response-file option may be
                 given any number of times, and is allowed in combination with
                 <file> arguments actually specified on the command-line.
                 Comments in the response file begin with '#' and extend to the
                 end of line.  Blank lines are ignored.
    -v --verbose Emit informational messages about the processing.  Can be
                 negated with --noverbose.  Deafult is --noverbose.
    -q --quiet   Suppress all output except for error messages.  Can be
                 negated with --noquiet.  Default is --noquiet.
    -V --version Display the version number of $PROG_NAME.
    -h --help    Display this usage message.

Project Options:
    <file>       Path of a file which belongs to the project represented by
                 this project.  Any number of files may be specified, or none
                 if the project contains no files.  Files with the suffixes .c,
                 .cc, .cpp, .C, .m, .mm, and .M are placed in the project's
                 "Source Files" group; files with the suffixes .h, .hpp, and .H
                 are placed in the "Header Files" group; and all other files
                 are placed in the "Resource Files" group.  Each mentioned file
                 replaces the variable \%file\% in the prjfile.tpi template.
    --output=<path>
                 Specifies the full path of the generated project file.  For an
                 MSVC 6 project file, the file extension should be '.dsp'.  For
                 an MSVC 7 project, it should be '.vcproj'.  This option is
                 mandatory.
    --template=<type>
                 Specifies the template type for this project.  The type may be
                 one of "appgui", "appcon", "plugin", "library", or "group".
                 See the discussion of project generation (above) for an
                 explanation of the various template types.  This option is
		 mandatory.
    --delaylib=<name>
                 Specifies the name of a DLL which should be delay-loaded.  In
                 prjdelay.tpi, \%delaylib\% is replaced with this value.  The
                 concatentation of each invocation of prjdelay.tpi becomes the
                 value of the \%delaylibs\% variable in the project template
                 (see --template).  The --delaylib option may be given any
                 number of times to specify any number of delay-loaded DLLs, or
                 not at all if no such DLLs are required.
    --fragment
    --fragment=<path>
                 Specifies whether or not to generate workspace fragment files
                 along with the project file.  Fragment files list the other
                 projects upon which this project relies, as well as other
                 information needed to fully synthesize workspace files.  If
                 not specified, then no fragments are generated.  If specified,
                 but no path is given, then the path (sans suffix) given with
                 --output is used as the fragment path.  Project, dependency,
                 and configuration fragment files are given the suffixes .pjf,
                 .dpf, and .cff, respectively.  The suffixes are added
                 automatically to the specified path.  Generated fragments can
                 later be incorporated into a workspace file collectively to
                 define a complete workspace for the entire project.  Each
                 dependency specified with the --depend option is listed in the
                 generated depedency fragment.
    --depend=<project>
                 Specifies the name of a project upon which this project
                 depends.  Each project name is written to the workspace
                 dependency fragment file for this project, and is the
                 replacement value for the \%depend\% variable in the wsdep.tpi
                 template.  The --depend option may be specified any number of
                 times, or not at all if the project has no dependencies (which
                 is often the case for "library" projects).  Dependencies are
                 subject to filtering by --accept and --reject.  This option
                 can be used only in conjunction with the --fragment option.
    --strip-root=<prefix>
                 It is generally wise for the source, header, and resource
                 files mentioned by the generated project file, and referenced
                 by the \%file\% interpolation variable, to be referenced by
                 paths relative to the root (or some other location within) the
                 project hierarchy, rather than by absolute paths.  This allows
                 the entire project source tree, along with the contained
                 project files, to be moved from location to location without
                 breakage.  Typically, this is accomplished by providing
                 relative pathnames for the files mentioned on the command-line
                 when the --project option is specified.  Alternately, if
                 absolute pathnames are given, then the --strip-root option can
                 be used to remove a prefix portion of each mentioned file.
                 The --strip-root option may be specified any number of times,
                 providing a different prefix on each occassion, or not at all.
                 if all mentioned files are specified via relative pathnames.

Workspace Options:
    <file>       Path of a fragment file emitted during a project generation
                 phase.  This is the basename for fragment files, as specified
                 via the --fragment option during project file generation.  The
                 name should not have a .pjf, .dpf, or .cff extension; the
                 appropriate extension will be added automatically to the
                 basename in order to locate the actual fragment files.  See
                 the discussion of fragment file generation (above) for
                 details.
    --output=<path>
                 Specifies the full path of the generated DSW/SLN workspace
                 file.  For an MSVC 6 workspace file, the file extension should
                 be '.dsw'.  For an MSVC 7 workspace, it should be '.sln'.
                 This option is mandatory.
EOT
}

#------------------------------------------------------------------------------
# Process command-line options.
#------------------------------------------------------------------------------
sub process_options {
    GetOptions(@script_options) or usage_error('');
    process_option_aliases();
    print_help() if $main::opt_help;
    print_version() if $main::opt_version;
    validate_options();
    process_global_options();
    process_workspace_options() if $main::opt_workspace;
    process_project_options() if $main::opt_project;
    banner() unless quiet();
}

sub print_version {
    banner(\*STDOUT);
    exit(0);
}

sub print_help {
    print_usage(\*STDOUT);
    exit(0);
}

sub usage_error {
    my $msg = shift;
    print STDERR "ERROR: $msg\n" if $msg;
    print STDERR "ERROR: Use the --help option for instructions.\n";
    exit(1);
}

#------------------------------------------------------------------------------
# Perform the complete repair process.
#------------------------------------------------------------------------------
process_options();
load_templates();
create_project() if $main::opt_project;
create_workspace() if $main::opt_workspace;
