#!/usr/bin/perl -w
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

use IO::File;
use IO::Socket;
use Net::DNS;
use Net::DNS::Packet;

my $localport = int($ENV{'PORT'});
if (!$localport) { $localport = 5300; }

my $sock = IO::Socket::INET->new(LocalAddr => "10.53.0.6",
   LocalPort => $localport, Proto => "udp") or die "$!";

my $pidf = new IO::File "ans.pid", "w" or die "cannot open pid file: $!";
print $pidf "$$\n" or die "cannot write pid file: $!";
$pidf->close or die "cannot close pid file: $!";
sub rmpid { unlink "ans.pid"; exit 1; };

$SIG{INT} = \&rmpid;
$SIG{TERM} = \&rmpid;

for (;;) {
	$sock->recv($buf, 512);

	print "**** request from " , $sock->peerhost, " port ", $sock->peerport, "\n";

	my $packet;

	if ($Net::DNS::VERSION > 0.68) {
		$packet = new Net::DNS::Packet(\$buf, 0);
		$@ and die $@;
	} else {
		my $err;
		($packet, $err) = new Net::DNS::Packet(\$buf, 0);
		$err and die $err;
	}

	print "REQUEST:\n";
	$packet->print;

	$packet->header->qr(1);

	my @questions = $packet->question;
	my $qname = $questions[0]->qname;
	my $qtype = $questions[0]->qtype;

	my $donotrespond = 0;

	$packet->header->aa(1);
	if ($qtype eq "A") {
		$packet->push("answer",
			      new Net::DNS::RR($qname .  " 300 A 10.53.0.5"));
	} else {
		$donotrespond = 1;
	}

	if ($donotrespond == 0) {
                my $sendsock =
                        IO::Socket::INET->new(LocalAddr => "10.53.1.2",
                                              PeerAddr => $sock->peerhost,
                                              PeerPort => $sock->peerport,
                                              Proto => "udp") or die "$!";
                print "**** response from ", $sendsock->sockhost, " to " ,
                      $sendsock->peerhost, " port ", $sendsock->peerport, "\n";
		$sendsock->send($packet->data);
                $sendsock->close;
		print "RESPONSE:\n";
		$packet->print;
		print "\n";
	} else {
		print "DROP:\n";
	}
}
