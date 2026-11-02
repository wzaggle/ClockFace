#!/usr/bin/env python3
"""Build the editable Word guide from USER.md. Requires python-docx."""
from pathlib import Path
import argparse,re
from docx import Document
from docx.shared import Inches,Pt,RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH,WD_TAB_ALIGNMENT
from docx.enum.table import WD_TABLE_ALIGNMENT,WD_CELL_VERTICAL_ALIGNMENT
from docx.oxml import OxmlElement
from docx.oxml.ns import qn

ROOT=Path(__file__).resolve().parents[1]

def el(tag,**attrs):
    node=OxmlElement(tag)
    for key,value in attrs.items():node.set(qn('w:'+key),str(value))
    return node

def inline(p,text):
    for part in re.split(r'(\*\*.*?\*\*|`[^`]+`)',text):
        if not part:continue
        r=p.add_run(part[2:-2] if part.startswith('**') else part[1:-1] if part.startswith('`') else part)
        r.font.color.rgb=RGBColor(0,0,0)
        if part.startswith('**'):r.bold=True
        if part.startswith('`'):
            r.font.name='Consolas';r.font.size=Pt(9.4)

def new_numbering(doc):
    numbering=doc.part.numbering_part.element
    ids=[int(n.get(qn('w:abstractNumId'))) for n in numbering.findall(qn('w:abstractNum'))]
    aid=max(ids,default=0)+1
    abstract=el('w:abstractNum',abstractNumId=aid)
    level=el('w:lvl',ilvl=0);level.append(el('w:start',val=1));level.append(el('w:numFmt',val='decimal'))
    level.append(el('w:lvlText',val='%1.'));level.append(el('w:lvlJc',val='left'))
    pp=el('w:pPr');pp.append(el('w:ind',left=330,hanging=300));level.append(pp);abstract.append(level);numbering.append(abstract)
    return aid

def new_list(doc,aid):
    numbering=doc.part.numbering_part.element
    ids=[int(n.get(qn('w:numId'))) for n in numbering.findall(qn('w:num'))]
    nid=max(ids,default=0)+1;num=el('w:num',numId=nid);num.append(el('w:abstractNumId',val=aid))
    override=el('w:lvlOverride',ilvl=0);override.append(el('w:startOverride',val=1));num.append(override);numbering.append(num)
    return nid

def table(doc,rows):
    t=doc.add_table(rows=1,cols=len(rows[0]));t.alignment=WD_TABLE_ALIGNMENT.CENTER;t.autofit=False
    widths=([1.65,2.20,3.15] if len(rows[0])==3 else [2.25,4.75])
    for col,width in zip(t.columns,widths):col.width=Inches(width)
    pr=t._tbl.tblPr;borders=el('w:tblBorders')
    for side in ['top','left','bottom','right','insideH','insideV']:borders.append(el('w:'+side,val='single',sz=4,color='D9D9D9'))
    pr.append(borders)
    margins=el('w:tblCellMar')
    for side,val in [('top',85),('bottom',85),('left',105),('right',105)]:margins.append(el('w:'+side,w=val,type='dxa'))
    pr.append(margins)
    for i,row in enumerate(rows):
        cells=t.rows[0].cells if i==0 else t.add_row().cells
        for j,(cell,text) in enumerate(zip(cells,row)):
            cell.width=Inches(widths[j]);cell.vertical_alignment=WD_CELL_VERTICAL_ALIGNMENT.CENTER
            cell._tc.get_or_add_tcPr().append(el('w:vAlign',val='center'))
            p=cell.paragraphs[0];p.paragraph_format.space_after=Pt(0);p.paragraph_format.line_spacing=1.04
            inline(p,text)
            for r in p.runs:r.font.size=Pt(10);r.bold=(i==0)
            if i==0:cell._tc.get_or_add_tcPr().append(el('w:shd',fill='EEF1F4'))
        trpr=t.rows[i]._tr.get_or_add_trPr();trpr.append(el('w:cantSplit'))
        if i==0:trpr.append(el('w:tblHeader'))
    doc.add_paragraph().paragraph_format.space_after=Pt(2)

def build(source,output):
    doc=Document();section=doc.sections[0]
    section.page_width=Inches(8.5);section.page_height=Inches(11)
    section.top_margin=Inches(.65);section.bottom_margin=Inches(.65)
    section.left_margin=Inches(.75);section.right_margin=Inches(.75)
    section.header_distance=Inches(.25);section.footer_distance=Inches(.28)
    normal=doc.styles['Normal'];normal.font.name='Calibri';normal.font.size=Pt(11);normal.font.color.rgb=RGBColor(0,0,0)
    normal.paragraph_format.space_after=Pt(7);normal.paragraph_format.line_spacing=1.10
    for name,size in [('Title',27),('Subtitle',12),('Heading 1',17),('Heading 2',12.5),('Heading 3',11.5)]:
        style=doc.styles[name];style.font.name='Calibri';style.font.size=Pt(size);style.font.color.rgb=RGBColor(0,0,0)
        style.font.bold=name!='Subtitle';style.paragraph_format.space_before=Pt(12 if name.startswith('Heading') else 0)
        style.paragraph_format.space_after=Pt(7);style.paragraph_format.keep_with_next=True
        for border in list(style.element.xpath('.//w:pBdr')):border.getparent().remove(border)
        for color in style.element.xpath('.//w:color'):
            for attr in ['themeColor','themeTint','themeShade']:
                color.attrib.pop(qn('w:'+attr),None)
    aid=new_numbering(doc);nid=None
    footer=section.footer.paragraphs[0];footer.paragraph_format.tab_stops.add_tab_stop(Inches(7),WD_TAB_ALIGNMENT.RIGHT)
    r=footer.add_run('OBS Clock '+(ROOT/'VERSION').read_text().strip()+'\tPage ');r.font.size=Pt(9)
    for kind in ['PAGE','NUMPAGES']:
        f=el('w:fldSimple',instr=kind);rnode=el('w:r');rpr=el('w:rPr');rpr.append(el('w:sz',val=18));rnode.append(rpr);text=el('w:t');text.text='1';rnode.append(text);f.append(rnode);footer._p.append(f)
        if kind=='PAGE':footer.add_run(' of ').font.size=Pt(9)
    for r in footer.runs:r.font.color.rgb=RGBColor(0,0,0)
    lines=source.read_text().splitlines();i=0;page=False
    while i<len(lines):
        line=lines[i].strip();i+=1
        if not line:continue
        if line=='<!-- page -->':page=True;continue
        if line.startswith('|'):
            raw=[line]
            while i<len(lines) and lines[i].strip().startswith('|'):raw.append(lines[i].strip());i+=1
            rows=[[v.strip() for v in row.strip('|').split('|')] for row in raw if not re.match(r'^\|\s*:?-+',row)]
            table(doc,rows);continue
        image=re.match(r'!\[(.*?)\]\((.*?)\)',line)
        if image:
            p=doc.add_paragraph();p.alignment=WD_ALIGN_PARAGRAPH.CENTER
            pic=p.add_run().add_picture(str(ROOT/image.group(2)),width=Inches(.78))
            pic._inline.docPr.set('descr',image.group(1));p.paragraph_format.space_after=Pt(7);continue
        if line.startswith('# '):
            p=doc.add_paragraph(line[2:],style='Title');p.alignment=WD_ALIGN_PARAGRAPH.CENTER;continue
        if line.startswith('## '):
            p=doc.add_paragraph(line[3:],style='Heading 1');p.paragraph_format.page_break_before=page;page=False;continue
        if line.startswith('### '):
            doc.add_paragraph(line[4:],style='Heading 2');continue
        match=re.match(r'^(\d+)\. (.*)',line)
        if match:
            if int(match.group(1))==1 or nid is None:nid=new_list(doc,aid)
            p=doc.add_paragraph();pp=p._p.get_or_add_pPr();np=el('w:numPr');np.append(el('w:ilvl',val=0));np.append(el('w:numId',val=nid));pp.append(np)
            inline(p,match.group(2));p.paragraph_format.space_after=Pt(6);continue
        p=doc.add_paragraph();inline(p,line)
        if line.startswith('**Version '):p.alignment=WD_ALIGN_PARAGRAPH.CENTER;p.paragraph_format.space_after=Pt(11)
    doc.core_properties.title='OBS Clock User Guide';doc.core_properties.subject='Platform installation and user instructions for OBS Clock 1.1.0'
    doc.core_properties.author='OBS Clock project';doc.core_properties.keywords='OBS Clock Windows macOS Linux user guide 1.1.0'
    output.parent.mkdir(parents=True,exist_ok=True);doc.save(output)

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,default=ROOT/'USER.md');parser.add_argument('--output',type=Path,default=ROOT/'docs/OBS-Clock-User-Guide-v1.1.0.docx')
    args=parser.parse_args();build(args.source,args.output);print(args.output)
