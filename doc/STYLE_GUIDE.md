# OpenWeave Documentation Style Guide

OpenWeave documentation lives in two locations:

*   **GitHub** — All guides and tutorials across the complete [OpenWeave organization](https://github/openweave).
*   **openweave.io** - OpenWeave news and features, educational material, API reference, and all guides and tutorials found on GitHub.

All documentation contributions are done on GitHub and mirrored on [openweave.io](https://openweave.io), and will be reviewed for clarity, accuracy, spelling, and grammar prior to acceptance.

See [CONTRIBUTING.md](../CONTRIBUTING.md) for general information on contributing to this project.

## Location

Place all documentation contributions in the appropriate location in the [`/doc`](./) directory. Most contributions should go into the `/doc/guides` subdirectory, which covers conceptual and usage content, to align it with the structure of the openweave.io documentation site.

Current documentation structure:

Directory | Description
----|----
`/doc/guides` | Conceptual or usage content that doesn't fit within a subdirectory, and high-level tutorials
`/doc/guides/images` | All images included in guide content
`/doc/guides/profiles` | Content describing or illustrating use of OpenWeave profiles
`/doc/guides/test` | Content related to testing Weave with Happy
`/doc/guides/tools` | Content describing or illustrating use of OpenWeave tools
`/doc/guides/weave-primer` | Weave Primer content
`/doc/images` | Top-level OpenWeave images, such as logos
`/doc/presentations` | PDF presentations on Weave features
`/doc/specs` | PDFs of Weave specifications

If you are unsure of the best location for your contribution, create an Issue and ask, or let us know in your Pull Request.

### Updating the site menus

When adding a new document, or moving one, also update the `_toc.yaml` file(s) in the related folders. For example, the `/doc/guides/tools/_toc.yaml` file is the menu for the [Tools section on openweave.io](https://openweave.io/guides/tools).

New documents should be added to the site menu TOCs where appropriate. If you are unsure of where to place a document within the menu, let us know in your Pull Request.

## Style

OpenWeave follows the [Google Developers Style Guide](https://developers.google.com/style/). See the [Highlights](https://developers.google.com/style/highlights) page for a quick overview.

## Links

For consistency, all document links should point to the content on GitHub, unless it refers to content only on [openweave.io](https://openweave.io).

The text of a link should be descriptive, so it's clear what the link is for:

> For more information, see the [OpenWeave Style Guide](./STYLE_GUIDE.md).

## Markdown guidelines

Use standard Markdown when authoring OpenWeave documentation. While HTML may be used for more complex content such as tables, use Markdown as much as possible. To ease mirroring and to keep formatting consistent with openweave.io, we ask that you follow the specific guidelines listed here.

> Note: Edit this file to see the Markdown behind the examples.

### Headers

The document title should be an h1 header (#) and in title case (all words are capitalized). All section headers should be h2 (##) or lower and in sentence case (only the first word and proper nouns are capitalized).

The best practice for document clarity is to not go lower than h3, but h4 is fine on occasion. Try to avoid using h4 often, or going lower than h4. If this happens, the document should be reorganized or broken up to ensure it stays at h3 with the occasional h4.

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

### Other prompts

The Device Manager prompt should also be used if necessary. It should start with `weave-device-mgr` and end with `>` to ensure it's properly imported into openweave.io:
```
weave-device-mgr > help
```

This includes verbose Device Manager prompts, such as when connected to another node:
```
weave-device-mgr (18B4300000000004 @ fd00:0:fab1:6:1ab4:3000:0:4) > ping
```

### Commands and output

All example commands and output should be in code blocks with backticks:

```
code in backticks
```

...unless the code is within a step list. In a step list, indent the code blocks:

    code indented

### Code blocks in step lists

When writing procedures that feature code blocks, indent the content for the code blocks:

1.	Step one:

        $ git clone https://github.com/openweave/openweave-core.git
        $ cd openweave-core

1.  Step two, do something else:

        $ ./configure

For clarity in instructions, avoid putting additional step commands after a code sample
within a step item. Instead rewrite the instruction so this is not necessary.

For example, avoid this:

1.  Step three, do this now:

        $ ./configure

    And then you will see that thing.

Instead, do this:

1.  Step three, do this now, and you will see that thing:

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

*   [GitHub version](./guides/cross-network-inet-multicast-howto.md)
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

### Material icons

The openweave.io documentation site uses various Material icons inline with the text or in
diagrams to represent elements of the Weave system. These icons are used to aid in your
understanding of Weave by highlighting common elements and are not official Weave-branded
icons.

> You are not required to use these icons, but we may ask to insert them during the review
process, in order to visually tie concepts together across the documentation site.

Example: https://openweave.io/guides/tools#weave-heartbeat

To use these icons in your content, so that they render on the openweave.io documentation site,
surround one of the supported keywords with an underscore+asterisk combo.

For example:

<div>_*Echo*_</div>
<div>_*heartbeat*_</div>

The keyword can be upper or lowercase. Use of this format with unsupported keywords merely
renders the text in italics.

See the **OpenWeave Tools** page for an example of this:

*   [GitHub version](./guides/tools/index.md)
*   [openweave.io version](https://openweave.io/guides/tools)

Supported keywords:

*	Echo
*	Heartbeat