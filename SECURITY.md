# Security and responsible disclosure

Lichii is an independent open-source emulator project. This public repository
must not be used to distribute copyrighted game images, console firmware,
console-derived keys, credentials, private user data, or raw game memory dumps.
Only contribute code and test artifacts you have the right to distribute.

## Reporting a security problem

If you discover a vulnerability involving secrets, code execution, or private
information, **do not publish exploit details, tokens, credentials, full memory
dumps, or identifiable log data in an issue or PR**. Use GitHub's private
vulnerability reporting feature under the repository's **Security** tab if
enabled; otherwise contact the maintainers privately through an established
out-of-band channel before sharing sensitive details.

If a credential is already exposed, revoke/rotate it immediately and
investigate its use. Merely deleting a file or adding a .gitignore entry
does not remove it from Git history, caches, forks, or clones.

## Before sharing diagnostics

- Redact personal names, machine/user directories, IP addresses, account IDs,
  authentication tokens and other identifiers from logs.
- Do not attach ROMs, game packages, firmware, console keys, or unreviewed
  shader/frame captures; these may embed copyrighted assets or private data.
- Prefer short reproduction instructions, code-level evidence and minimal,
  sanitized logs.
- Double-check third-party changes for their copyright notices, permissions
  and licenses. Preserve required attribution and source availability.

Automated CI reviews and secret-pattern searches are useful safeguards, not
proof that a release is lawful or secure. Do not enable auto-merge for bot
feedback alone.

The usual source licenses continue to apply; see [LICENSE.txt](LICENSE.txt)
and individual source headers.
