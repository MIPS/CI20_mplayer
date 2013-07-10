<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">

<!-- $Id: ldp-html.xsl,v 1.1.1.1 2012/03/27 04:02:57 dqliu Exp $  -->

<!-- This stylesheet calls Norman Walsh's 'docbook.xsl' stylesheet
     and therefore generates a SINGLE HTML FILE as output. -->

<!-- Note the the *order* of the import statements below is important and
     should not be changed. -->

<!-- Change this to the path to where you have installed Norman
     Walsh's XSL stylesheets.  -->
<xsl:import href="/usr/share/sgml/docbook/xsl-stylesheets-1.60.1/html/docbook.xsl"/>

<!-- Imports the common LDP customization layer. -->
<xsl:import href="/home/n/xml/xsl/ldp-html-common.xsl"/>

<!-- If there was some reason to override 'ldp-html-common.xsl' or to
     perform any other customizations that affect *only* the generation
     of a single HTML file, those templates or parameters could be
     entered here. -->

</xsl:stylesheet>
