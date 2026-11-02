# Maintaining the user guide

USER.md is the editable guide text. `<!-- page -->` markers separate deliberate
Word pages; Markdown headings, simple tables, numbered steps, bold text, inline
code, and the icon are supported by the small document builder.

Install Python 3 and python-docx in a development environment, then run:

```shell
python tools/build_user_guide.py
```

Review the resulting Word document. Keep version metadata in USER.md, VERSION,
the generator's output default, and documentation links aligned for a new release.
The current Word guide has been rendered and visually checked before packaging.

For a PDF, use Word's Save As/Export to PDF. If LibreOffice is installed on PATH,
`tools/export_guide_pdf.ps1` offers a convenience conversion. Inspect every page
of the resulting PDF before including it in a release.

Do not treat the concept illustration as a product screenshot. Keep clock settings
and installation paths grounded in the actual current source. Review docs/TESTING.md
when distinguishing verified behavior from acceptance scenarios still to record.
