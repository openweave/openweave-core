# OpenWeave Documentation Style Guide

OpenWeave documentation lives in two locations:

*   **GitHub** — All guides and tutorials across the complete [OpenWeave organization](https://github/openweave).
*   **openweave.io** - OpenWeave news and features, educational material, API reference, and all guides and tutorials found on GitHub.

All documentation contributions are done on GitHub and mirrored on [openweave.io](https://openweave.io), and will be reviewed for clarity, accuracy, spelling, and grammar prior to acceptance.

See [CONTRIBUTING.md](../CONTRIBUTING.md) for general information on contributing to this project.

## Location

Place all documentation contributions in the appropriate location in the [`/doc`](./) directory. If you are unsure of the best location for your contribution, let us know in your pull request.

## Style

OpenWeave follows the [Google Developers Style Guide](https://developers.google.com/style/). See the [Highlights](https://developers.google.com/style/highlights) page for a quick overview.

## Markdown guidelines

Use standard Markdown when authoring OpenWeave documentation. While HTML may be used for more complex content such as tables, use Markdown as much as possible. To ease mirroring and to keep formatting consistent with openweave.io, we ask that you follow the specific guidelines listed here.

> Note: Edit this file to see the Markdown behind the examples.

### Command line examples

Feel free to use either `$` or `%` to preface command line examples, but be consistent within the same doc or set of docs:

```
$ git clone https://github.com/openweave/openweave-core.git
% git clone https://github.com/openweave/openweave-core.git
```

### Terminal prompts

If you need use a full terminal prompt with username and hostname, use the format of `root@{hostname}{special-characters}#`.

For example, when logged into a Docker container, you might have a prompt like this:
```
root@c0f3912a74ff:/#
```

Or in a Happy node, you might have:
```
root@BorderRouter:#
```

### Commands and output

All example commands and output should be in code blocks with backticks or indented:

```
code in backticks
```

    code indented

### Code blocks in step lists

When writing procedures that feature code blocks, indent the content for the code blocks:

1.	Step one.

        $ git clone https://github.com/openweave/openweave-core.git
        $ cd openweave-core

    More about step one.

1.  Step two, do something else.

        $ ./configure

### Inline code

Use backticks for `inline code`. This includes file paths and file or binary names.

### Replaceable variables

In code examples, denote a replaceable variable with curly braces:

```
--with-weave-inet-project-includes={directory}        
```

### Step header numbers

If you want your headers to render with nice-looking step numbers on openweave.io, use level 2 Markdown headers in this format:

    ## Step 1: Text of the step
    ## Step 2: Text of the next step
    ## Step 3: Text of the last step

See the **OpenWeave + Happy Cross Network Multicast Inet Layer HOWTO** for an example of this:

*   [GitHub version](/guides/cross-network-inet-multicast-howto.md)
*   [openweave.io version](https://openweave.io/guides/cross-network-inet-multicast-howto)

### Callouts

Use a blockquote `>` with one of these callout types:

* 	Note
*   Caution
*   Warning

For example:

> Note: This is something to be aware of.

Or:

> Caution: The user should be careful running the next command.